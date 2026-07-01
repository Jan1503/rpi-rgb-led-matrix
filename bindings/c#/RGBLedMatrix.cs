using System.Buffers;
using System.Runtime.InteropServices;

namespace RPiRgbLEDMatrix;

/// <summary>
/// Represents a RGB matrix.
/// </summary>
public class RGBLedMatrix : IDisposable
{
    private IntPtr matrix;
    private bool disposedValue = false;

    // Native RGBMatrix::Options keeps the raw string pointers we pass in and
    // re-reads them on every CreateFrameCanvas()/CreateOffscreenCanvas() (e.g.
    // panel_type + led_rgb_sequence). We must therefore keep these HGlobal
    // allocations alive for the whole lifetime of the matrix and only free them
    // on Dispose -- freeing them right after create is a use-after-free that
    // corrupts panel_type on the next canvas (crashes 128-row SPWM panels).
    private IntPtr _hardwareMappingPtr;
    private IntPtr _ledRgbSequencePtr;
    private IntPtr _pixelMapperConfigPtr;
    private IntPtr _panelTypePtr;

    private void FreeOptionStrings()
    {
        if (_hardwareMappingPtr != IntPtr.Zero) { Marshal.FreeHGlobal(_hardwareMappingPtr); _hardwareMappingPtr = IntPtr.Zero; }
        if (_ledRgbSequencePtr != IntPtr.Zero) { Marshal.FreeHGlobal(_ledRgbSequencePtr); _ledRgbSequencePtr = IntPtr.Zero; }
        if (_pixelMapperConfigPtr != IntPtr.Zero) { Marshal.FreeHGlobal(_pixelMapperConfigPtr); _pixelMapperConfigPtr = IntPtr.Zero; }
        if (_panelTypePtr != IntPtr.Zero) { Marshal.FreeHGlobal(_panelTypePtr); _panelTypePtr = IntPtr.Zero; }
    }

    /// <summary>
    /// Initializes a new matrix.
    /// </summary>
    /// <param name="rows">Size of a single module. Can be 32, 16 or 8.</param>
    /// <param name="chained">How many modules are connected in a chain.</param>
    /// <param name="parallel">How many modules are connected in a parallel.</param>
    public RGBLedMatrix(int rows, int chained, int parallel)
    {
        matrix = led_matrix_create(rows, chained, parallel);
        if (matrix == (IntPtr)0)
            throw new ArgumentException("Could not initialize a new matrix");
    }

    /// <summary>
    /// Initializes a new matrix.
    /// </summary>
    /// <param name="options">A configuration of a matrix.</param>
    public RGBLedMatrix(RGBLedMatrixOptions options)
    {
        InternalRGBLedMatrixOptions opt = default;
        try
        {
            opt = new(options);
            var args = Environment.GetCommandLineArgs();

            // Because gpio-slowdown is not provided in the options struct,
            // we manually add it.
            // Let's add it first to the command-line we pass to the
            // matrix constructor, so that it can be overridden with the
            // users' commandline.
            // As always, as the _very_ first, we need to provide the
            // program name argv[0].
            // --led-no-drop-privs is also added as there seems to be no other way to do it
            // other than passing it via command-line, and if you call the bindings from a library,
            // you cannot do that unless you also pass that argument to your app, which just seems silly.
            // Without no-drop-privs, dotnet cannot load any libraries, so it seems essential
            var argv = new string[args.Length + 3];
            var argIndex = 0;
            argv[argIndex++] = args[0];
            argv[argIndex++] = $"--led-slowdown-gpio={options.GpioSlowdown}";
            argv[argIndex++] = $"--led-rp1-pio={options.Rp1Pio}";
            argv[argIndex++] = "--led-no-drop-privs";
            Array.Copy(args, 1, argv, argIndex, args.Length - 1);

            // Keep the marshalled option strings alive for the matrix lifetime;
            // native retains these pointers (see field docs above).
            _hardwareMappingPtr = opt.hardware_mapping;
            _ledRgbSequencePtr = opt.led_rgb_sequence;
            _pixelMapperConfigPtr = opt.pixel_mapper_config;
            _panelTypePtr = opt.panel_type;

            matrix = led_matrix_create_from_options_const_argv(ref opt, argv.Length, argv);
            if (matrix == (IntPtr)0)
            {
                FreeOptionStrings();
                throw new ArgumentException("Could not initialize a new matrix");
            }
        }
        catch
        {
            FreeOptionStrings();
            throw;
        }
    }

    /// <summary>
    /// Creates a new backbuffer canvas for drawing on.
    /// </summary>
    /// <returns>An instance of <see cref="RGBLedCanvas"/> representing the canvas.</returns>
    public RGBLedCanvas CreateOffscreenCanvas() => new(led_matrix_create_offscreen_canvas(matrix));

    [System.Runtime.InteropServices.DllImport("librgbmatrix.so.1")]
    private static extern void framebuffer_reset_globals();

    /// <summary>
    /// Reset native framebuffer globals so that GPIO/row-address state will be
    /// reinitialized on the next matrix creation. Useful when changing
    /// hardware-mapping critical options at runtime.
    /// </summary>
    public static void ResetFramebufferGlobals() => framebuffer_reset_globals();

    /// <summary>
    /// Returns a canvas representing the current frame buffer.
    /// </summary>
    /// <returns>An instance of <see cref="RGBLedCanvas"/> representing the canvas.</returns>
    /// <remarks>Consider using <see cref="CreateOffscreenCanvas"/> instead.</remarks>
    public RGBLedCanvas GetCanvas() => new(led_matrix_get_canvas(matrix));

    /// <summary>
    /// Swaps this canvas with the currently active canvas. The active canvas
    /// becomes a backbuffer and is mapped to <paramref name="canvas"/> instance.
    /// <br/>
    /// This operation guarantees vertical synchronization.
    /// </summary>
    /// <param name="canvas">Backbuffer canvas to swap.</param>
    public void SwapOnVsync(RGBLedCanvas canvas)
    {
        if (canvas is RGBLedCanvas ca)
        {
            ca._canvas = led_matrix_swap_on_vsync(matrix, ca._canvas);
        }
        else
        {
            throw new ArgumentException("Does not support implementation other than RGBLedCanvas");
        }
    }

    /// <summary>
    /// The general brightness of the matrix.
    /// </summary>
    public byte Brightness
    {
        get => led_matrix_get_brightness(matrix);
        set => led_matrix_set_brightness(matrix, value);
    }

    protected virtual void Dispose(bool disposing)
    {
        if (disposedValue) return;

        led_matrix_delete(matrix);
        // Native no longer references the option strings once the matrix is
        // deleted; release the HGlobal allocations we kept alive for it.
        FreeOptionStrings();
        disposedValue = true;
    }

    ~RGBLedMatrix() => Dispose(false);

    /// <inheritdoc/>
    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }
}
