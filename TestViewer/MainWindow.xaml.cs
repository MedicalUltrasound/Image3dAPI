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
            ushort[] max_res = new ushort[] { 200, 300, 250 };

            // get XY plane
            Cart3dGeom bboxXY = m_source.GetBoundingBox();
            ushort[] max_resXY = new ushort[] { max_res[0], max_res[1], 1 };
            bboxXY.origin_x = bboxXY.origin_x + bboxXY.dir3_x / 2;
            bboxXY.origin_y = bboxXY.origin_y + bboxXY.dir3_y / 2;
            bboxXY.origin_z = bboxXY.origin_z + bboxXY.dir3_z / 2;
            bboxXY.dir3_x = 0;
            bboxXY.dir3_y = 0;
            bboxXY.dir3_z = 0;
            Image3d imageXY = m_source.GetFrame(frame, bboxXY, max_resXY);

            // get XZ plane
            Cart3dGeom bboxXZ = m_source.GetBoundingBox();
            ushort[] max_resXZ = new ushort[] { max_res[0], max_res[2], 1 };
            bboxXZ.origin_x = bboxXZ.origin_x + bboxXZ.dir2_x / 2;
            bboxXZ.origin_y = bboxXZ.origin_y + bboxXZ.dir2_y / 2;
            bboxXZ.origin_z = bboxXZ.origin_z + bboxXZ.dir2_z / 2;
            bboxXZ.dir2_x = bboxXZ.dir3_x;
            bboxXZ.dir2_y = bboxXZ.dir3_y;
            bboxXZ.dir2_z = bboxXZ.dir3_z;
            bboxXZ.dir3_x = 0; 
            bboxXZ.dir3_y = 0;
            bboxXZ.dir3_z = 0;
            Image3d imageXZ = m_source.GetFrame(frame, bboxXZ, max_resXZ);

            // get YZ plane
            Cart3dGeom bboxYZ = m_source.GetBoundingBox();
            ushort[] max_resYZ = new ushort[] { max_res[1], max_res[2], 1 };
            bboxYZ.origin_x = bboxYZ.origin_x + bboxYZ.dir1_x / 2;
            bboxYZ.origin_y = bboxYZ.origin_y + bboxYZ.dir1_y / 2;
            bboxYZ.origin_z = bboxYZ.origin_z + bboxYZ.dir1_z / 2;
            bboxYZ.dir1_x = bboxYZ.dir2_x;
            bboxYZ.dir1_y = bboxYZ.dir2_y;
            bboxYZ.dir1_z = bboxYZ.dir2_z;
            bboxYZ.dir2_x = bboxYZ.dir3_x;
            bboxYZ.dir2_y = bboxYZ.dir3_y;
            bboxYZ.dir2_z = bboxYZ.dir3_z;
            bboxYZ.dir3_x = 0; 
            bboxYZ.dir3_y = 0;
            bboxYZ.dir3_z = 0;
            Image3d imageYZ = m_source.GetFrame(frame, bboxYZ, max_resYZ);

            FrameTime.Text = "Frame time: " + imageXY.time;

            uint[] color_map = m_source.GetColorMap();

            {
                // extract center-Z slize (top-left)
                WriteableBitmap bitmap = new WriteableBitmap(imageXY.dims[0], imageXY.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe
                {
                    for (int y = 0; y < bitmap.Height; ++y)
                    {
                        for (int x = 0; x < bitmap.Width; ++x)
                        {
                            byte val = imageXY.data[x + y * imageXY.stride0];
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
                WriteableBitmap bitmap = new WriteableBitmap(imageXZ.dims[0], imageXZ.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe
                {
                    for (int z = 0; z < bitmap.Height; ++z)
                    {
                        for (int x = 0; x < bitmap.Width; ++x)
                        {
                            byte val = imageXZ.data[x + z * imageXZ.stride0];
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
                WriteableBitmap bitmap = new WriteableBitmap(imageYZ.dims[0], imageYZ.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
                bitmap.Lock();
                unsafe {
                    for (int z = 0; z < bitmap.Height; ++z)
                    {
                        for (int y = 0; y < bitmap.Width; ++y)
                        {
                            byte val = imageYZ.data[y + z * imageYZ.stride0];
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
