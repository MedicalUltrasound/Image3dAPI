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

        void ClearUI()
        {
            FrameSelector.Minimum = 0;
            FrameSelector.Maximum = 0;
            FrameSelector.IsEnabled = false;
            FrameSelector.Value = 0;

            FrameCount.Text = "";
            ProbeInfo.Text = "";
            InstanceUID.Text = "";


            ImageXY.Source = null;
            ImageXZ.Source = null;
            ImageYZ.Source = null;
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

            // API version compatibility check
            RegistryKey ver_key = Registry.ClassesRoot.OpenSubKey("CLSID\\{"+comType.GUID+"}\\Version");
            string ver_str = (string)ver_key.GetValue("");
            string cur_ver = string.Format("{0}.{1}", (int)Image3dAPIVersion.IMAGE3DAPI_VERSION_MAJOR, (int)Image3dAPIVersion.IMAGE3DAPI_VERSION_MINOR);
            if (ver_str != cur_ver) {
                MessageBox.Show(string.Format("Incompatible loader version. Loader uses version {0}, while the current version is {1}.", ver_str, cur_ver));
                return;
            }

            // clear UI when switching to a new loader
            ClearUI();

            m_loader = (IImage3dFileLoader)Activator.CreateInstance(comType);

            this.FileOpenBtn.IsEnabled = true;
        }

        private void FileSelectBtn_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            if (dialog.ShowDialog() != true)
                return; // user hit cancel

            // clear UI when opening a new file
            ClearUI();

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
            FrameSelector.Value = 0;

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

            // retrieve image slices
            const ushort HORIZONTAL_RES = 256;
            const ushort VERTICAL_RES = 256;

            Cart3dGeom bbox = m_source.GetBoundingBox();
            if (Math.Abs(bbox.dir3_y) > Math.Abs(bbox.dir2_y)){
                // swap 2nd & 3rd axis, so that the 2nd becomes predominately "Y"
                SwapVals(ref bbox.dir2_x, ref bbox.dir3_x);
                SwapVals(ref bbox.dir2_y, ref bbox.dir3_y);
                SwapVals(ref bbox.dir2_z, ref bbox.dir3_z);
            }

            // extend bounding-box axes, so that dir1, dir2 & dir3 have equal length
            ExtendBoundingBox(ref bbox);

            // get XY plane (assumes 1st axis is "X" and 2nd is "Y")
            Cart3dGeom bboxXY = bbox;
            bboxXY.origin_x = bboxXY.origin_x + bboxXY.dir3_x / 2;
            bboxXY.origin_y = bboxXY.origin_y + bboxXY.dir3_y / 2;
            bboxXY.origin_z = bboxXY.origin_z + bboxXY.dir3_z / 2;
            bboxXY.dir3_x = 0;
            bboxXY.dir3_y = 0;
            bboxXY.dir3_z = 0;
            Image3d imageXY = m_source.GetFrame(frame, bboxXY, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });

            // get XZ plane (assumes 1st axis is "X" and 3rd is "Z")
            Cart3dGeom bboxXZ = bbox;
            bboxXZ.origin_x = bboxXZ.origin_x + bboxXZ.dir2_x / 2;
            bboxXZ.origin_y = bboxXZ.origin_y + bboxXZ.dir2_y / 2;
            bboxXZ.origin_z = bboxXZ.origin_z + bboxXZ.dir2_z / 2;
            bboxXZ.dir2_x = bboxXZ.dir3_x;
            bboxXZ.dir2_y = bboxXZ.dir3_y;
            bboxXZ.dir2_z = bboxXZ.dir3_z;
            bboxXZ.dir3_x = 0; 
            bboxXZ.dir3_y = 0;
            bboxXZ.dir3_z = 0;
            Image3d imageXZ = m_source.GetFrame(frame, bboxXZ, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });

            // get YZ plane (assumes 2nd axis is "Y" and 3rd is "Z")
            Cart3dGeom bboxYZ = bbox;
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
            Image3d imageYZ = m_source.GetFrame(frame, bboxYZ, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });

            FrameTime.Text = "Frame time: " + imageXY.time;

            uint[] color_map = m_source.GetColorMap();

            ImageXY.Source = GenerateBitmap(imageXY, color_map);
            ImageXZ.Source = GenerateBitmap(imageXZ, color_map);
            ImageYZ.Source = GenerateBitmap(imageYZ, color_map);
        }

        private WriteableBitmap GenerateBitmap(Image3d image, uint[] color_map)
        {
            WriteableBitmap bitmap = new WriteableBitmap(image.dims[0], image.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
            bitmap.Lock();
            unsafe
            {
                for (int y = 0; y < bitmap.Height; ++y)
                {
                    for (int x = 0; x < bitmap.Width; ++x)
                    {
                        byte val = image.data[x + y * image.stride0];
                        byte* pixel = (byte*)bitmap.BackBuffer + x * (bitmap.Format.BitsPerPixel / 8) + y * bitmap.BackBufferStride;
                        SetRGBVal(pixel, color_map[val]);
                    }
                }
            }
            bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
            bitmap.Unlock();
            return bitmap;
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

        static void SwapVals(ref float v1, ref float v2)
        {
            float tmp = v1;
            v1 = v2;
            v2 = tmp;
        }

        static float VecLen(float x, float y, float z)
        {
            return (float)Math.Sqrt(x * x + y * y + z * z);
        }

        static float VecLen(Cart3dGeom g, int idx)
        {
            if (idx == 1)
                return VecLen(g.dir1_x, g.dir1_y, g.dir1_z);
            else if (idx == 2)
                return VecLen(g.dir2_x, g.dir2_y, g.dir2_z);
            else if (idx == 3)
                return VecLen(g.dir3_x, g.dir3_y, g.dir3_z);

            throw new Exception("unsupported direction index");
        }

        /** Scale bounding-box, so that all axes share the same length.
         *  Also update the origin to keep the bounding-box centered. */
        static void ExtendBoundingBox(ref Cart3dGeom g)
        {
            float dir1_len = VecLen(g, 1);
            float dir2_len = VecLen(g, 2);
            float dir3_len = VecLen(g, 3);

            float max_len = Math.Max(dir1_len, Math.Max(dir2_len, dir3_len));

            if (dir1_len < max_len)
            {
                float delta = max_len - dir1_len;
                float dx, dy, dz;
                ScaleVector(g.dir1_x, g.dir1_y, g.dir1_z, delta, out dx, out dy, out dz);
                // scale up dir1 so that it becomes the same length as the other axes
                g.dir1_x += dx;
                g.dir1_y += dy;
                g.dir1_z += dz;
                // move origin to keep the bounding-box centered
                g.origin_x -= dx/2;
                g.origin_y -= dy/2;
                g.origin_z -= dz/2;
            }

            if (dir2_len < max_len)
            {
                float delta = max_len - dir2_len;
                float dx, dy, dz;
                ScaleVector(g.dir2_x, g.dir2_y, g.dir2_z, delta, out dx, out dy, out dz);
                // scale up dir2 so that it becomes the same length as the other axes
                g.dir2_x += dx;
                g.dir2_y += dy;
                g.dir2_z += dz;
                // move origin to keep the bounding-box centered
                g.origin_x -= dx / 2;
                g.origin_y -= dy / 2;
                g.origin_z -= dz / 2;
            }

            if (dir3_len < max_len)
            {
                float delta = max_len - dir3_len;
                float dx, dy, dz;
                ScaleVector(g.dir3_x, g.dir3_y, g.dir3_z, delta, out dx, out dy, out dz);
                // scale up dir3 so that it becomes the same length as the other axes
                float factor = max_len / dir3_len;
                g.dir3_x += dx;
                g.dir3_y += dy;
                g.dir3_z += dz;
                // move origin to keep the bounding-box centered
                g.origin_x -= dx / 2;
                g.origin_y -= dy / 2;
                g.origin_z -= dz / 2;
            }
        }

        static void ScaleVector(float in_x, float in_y, float in_z, float length, out float out_x, out float out_y, out float out_z)
        {
            float cur_len = VecLen(in_x, in_y, in_z);
            out_x = in_x * length / cur_len;
            out_y = in_y * length / cur_len;
            out_z = in_z * length / cur_len;
        }
    }
}
