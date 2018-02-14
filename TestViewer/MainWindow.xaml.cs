using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Diagnostics;
using System.Windows.Controls.Primitives;
using Microsoft.Win32;
using Image3dAPI;


namespace TestViewer
{
    public partial class MainWindow : Window
    {
        IImage3dFileLoader m_loader;
        IImage3dSource     m_source;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void LoadBtn_Click(object sender, RoutedEventArgs e)
        {
            // try to parse string as ProgId first
            Type comType = Type.GetTypeFromProgID(LoaderName.Text);
            if (comType == null) {
                try {
                    // fallback to parse string as CLSID hex value
                    Guid guid = Guid.Parse(LoaderName.Text);
                    comType = Type.GetTypeFromCLSID(guid);
                } catch (FormatException) {
                    MessageBox.Show("Unknown ProgId of CLSID.");
                    return;
                }
            }

            m_loader = (IImage3dFileLoader)Activator.CreateInstance(comType);

            this.FileOpenBtn.IsEnabled = true;
        }

        private void FileSelectBtn_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            if (dialog.ShowDialog() == true)
                FileName.Text = dialog.FileName;
        }

        private void FileOpenBtn_Click(object sender, RoutedEventArgs e)
        {
            Debug.Assert(m_loader != null);

            string loader_error = m_loader.LoadFile(FileName.Text);
            m_source = m_loader.GetImageSource();

            FrameSelector.Minimum = 0;
            FrameSelector.Maximum = m_source.GetFrameCount()-1;
            FrameSelector.IsEnabled = true;

            FrameCount.Text = "Frame count: " + m_source.GetFrameCount();
            ProbeInfo.Text = "Probe name: "+ m_source.GetProbeInfo().name;
            InstanceUID.Text = "UID: " + m_source.GetSopInstanceUID();

            DrawImages(0);
        }

        private void FrameSelector_ValueChanged(object sender, DragCompletedEventArgs e)
        {
            DrawImages((uint)FrameSelector.Value);
        }

        private void DrawImages (uint frame)
        {
            Debug.Assert(m_source != null);

            // retrieve image volume
            Cart3dGeom bbox  = m_source.GetBoundingBox();
            ushort[] max_res = new ushort[]{ 128, 128, 128 };
            Image3d image = m_source.GetFrame(frame, bbox, max_res);

            FrameTime.Text = "Frame time: " + image.time;

            uint[] color_map = m_source.GetColorMap();

            {
                // extract center-Z slize (top-left)
                WriteableBitmap bitmap = new WriteableBitmap(image.dims[0], image.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe
                {
                    int z = image.dims[2] / 2;
                    for (int y = 0; y < bitmap.Height; ++y)
                    {
                        for (int x = 0; x < bitmap.Width; ++x)
                        {
                            byte val = image.data[x + y * image.stride0 + z * image.stride1];
                            byte* pixel = (byte*)bitmap.BackBuffer + x * (bitmap.Format.BitsPerPixel / 8) + y * bitmap.BackBufferStride;
                            SetRGBVal(pixel, color_map[val]);
                        }
                    }
                }
                bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
                bitmap.Unlock();

                ImageXY.Source = bitmap;
            }
            {
                // extract center-Y slize (top-right)
                WriteableBitmap bitmap = new WriteableBitmap(image.dims[0], image.dims[2], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe
                {
                    int y = image.dims[1] / 2;
                    for (int z = 0; z < bitmap.Height; ++z)
                    {
                        for (int x = 0; x < bitmap.Width; ++x)
                        {
                            byte val = image.data[x + y * image.stride0 + z * image.stride1];
                            byte* pixel = (byte*)bitmap.BackBuffer + x * (bitmap.Format.BitsPerPixel / 8) + z * bitmap.BackBufferStride;
                            SetRGBVal(pixel, color_map[val]);
                        }
                    }
                }
                bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
                bitmap.Unlock();

                ImageXZ.Source = bitmap;
            }
            {
                // extract center-X slize (bottom-left)
                WriteableBitmap bitmap = new WriteableBitmap(image.dims[2], image.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe {
                    int x = image.dims[0] / 2;
                    for (int z = 0; z < bitmap.Height; ++z)
                    {
                        for (int y = 0; y < bitmap.Width; ++y)
                        {
                            byte val = image.data[x + y * image.stride0 + z * image.stride1];
                            byte* pixel = (byte*)bitmap.BackBuffer + y * (bitmap.Format.BitsPerPixel / 8) + z * bitmap.BackBufferStride;
                            SetRGBVal(pixel, color_map[val]);
                        }
                    }
                }
                bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
                bitmap.Unlock();

                ImageYZ.Source = bitmap;
            }
        }

        unsafe static void SetRGBVal(byte* pixel, uint rgba)
        {
            // split input rgba color into individual channels
            byte* channels = (byte*)&rgba;
            // assign red, green & blue
            pixel[0] = channels[0]; // red
            pixel[1] = channels[1]; // green
            pixel[2] = channels[2]; // blue
            // discard alpha channel
        }
    }
}
