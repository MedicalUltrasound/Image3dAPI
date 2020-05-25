﻿using System;
using System.Linq;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using Image3dAPI;


/** Alternative to System.Activator.CreateInstance that allows explicit control over the activation context.
 *  Based on https://stackoverflow.com/questions/22901224/hosting-managed-code-and-garbage-collection */
public static class ComExt {
    [DllImport("ole32.dll", ExactSpelling = true, PreserveSig = false)]
    static extern void CoCreateInstance(
       [MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
       [MarshalAs(UnmanagedType.IUnknown)] object pUnkOuter,
       uint dwClsContext,
       [MarshalAs(UnmanagedType.LPStruct)] Guid riid,
       [MarshalAs(UnmanagedType.Interface)] out object rReturnedComObject);

    public static object CreateInstance(Guid clsid, bool force_out_of_process) {
        Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");
        const uint CLSCTX_INPROC_SERVER = 0x1;
        const uint CLSCTX_LOCAL_SERVER  = 0x4;

        uint class_context = CLSCTX_LOCAL_SERVER; // always allow out-of-process activation
        if (!force_out_of_process)
            class_context |= CLSCTX_INPROC_SERVER; // allow in-process activation

        object unk;
        CoCreateInstance(clsid, null, class_context, IID_IUnknown, out unk);
        return unk;
    }
}


namespace TestViewer
{
    public partial class MainWindow : Window
    {
        IImage3dFileLoader m_loader;
        IImage3dSource     m_source;

        IImage3dStream     m_streamXY;
        IImage3dStream     m_streamXYcf;
        IImage3dStream     m_streamXZ;
        IImage3dStream     m_streamXZcf;
        IImage3dStream     m_streamZY;
        IImage3dStream     m_streamZYcf;

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
            ImageZY.Source = null;

            m_streamXY = null;
            m_streamXYcf = null;
            m_streamXZ = null;
            m_streamXZcf = null;
            m_streamZY = null;
            m_streamZYcf = null;

            ECG.Data = null;

            if (m_source != null) {
                Marshal.ReleaseComObject(m_source);
                m_source = null;
            }
        }

        private void LoadDefaultBtn_Click(object sender, RoutedEventArgs e)
        {
            LoadImpl(false);
        }

        private void LoadOutOfProcBtn_Click(object sender, RoutedEventArgs e)
        {
            LoadImpl(true);
        }

        private void LoadImpl(bool force_out_of_proc)
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
            try {
                RegistryKey ver_key = Registry.ClassesRoot.OpenSubKey("CLSID\\{" + comType.GUID + "}\\Version");
                string ver_str = (string)ver_key.GetValue("");
                string cur_ver = string.Format("{0}.{1}", (int)Image3dAPIVersion.IMAGE3DAPI_VERSION_MAJOR, (int)Image3dAPIVersion.IMAGE3DAPI_VERSION_MINOR);
                if (ver_str != cur_ver) {
                    MessageBox.Show(string.Format("Loader uses version {0}, while the current version is {1}.", ver_str, cur_ver), "Incompatible loader version");
                    return;
                }
            } catch (Exception err) {
                MessageBox.Show(err.Message, "Version check error");
                // continue, since this error will also appear if the loader has non-matching bitness
            }

            // clear UI when switching to a new loader
            ClearUI();

            if (m_loader != null)
                Marshal.ReleaseComObject(m_loader);
            m_loader = (IImage3dFileLoader)ComExt.CreateInstance(comType.GUID, force_out_of_proc);

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

            Image3dError err_type = Image3dError.Image3d_SUCCESS;
            string err_msg = "";
            try {
                m_loader.LoadFile(FileName.Text, out err_type, out err_msg);
            } catch (Exception) {
                // NOTE: err_msg does not seem to be marshaled back on LoadFile failure in .Net.
                // NOTE: This problem is limited to .Net, and does not occur in C++

                string message = "Unknown error";
                if ((err_type != Image3dError.Image3d_SUCCESS)) {
                    switch (err_type) {
                        case Image3dError.Image3d_ACCESS_FAILURE:
                            message = "Unable to open the file. The file might be missing or locked.";
                            break;
                        case Image3dError.Image3d_VALIDATION_FAILURE:
                            message = "Unsupported file. Probably due to unsupported vendor or modality.";
                            break;
                        case Image3dError.Image3d_NOT_YET_SUPPORTED:
                            message = "The loader is too old to parse the file.";
                            break;
                        case Image3dError.Image3d_SUPPORT_DISCONTINUED:
                            message = "The the file version is no longer supported (pre-DICOM format?).";
                            break;
                    }
                }
                MessageBox.Show("LoadFile error: " + message + " (" + err_msg+")");
                return;
            }

            try {
                if (m_source != null)
                    Marshal.ReleaseComObject(m_source);
                m_source = m_loader.GetImageSource();
            } catch (Exception err) {
                MessageBox.Show("ERROR: " + err.Message, "GetImageSource error");
                return;
            }

            InitializeSlices();

            FrameSelector.Minimum = 0;
            FrameSelector.Maximum = m_streamXY.GetFrameCount()-1;
            FrameSelector.IsEnabled = true;
            FrameSelector.Value = 0;

            FrameCount.Text = "Frame count: " + m_streamXY.GetFrameCount();
            ProbeInfo.Text = "Probe name: "+ m_source.GetProbeInfo().name;
            InstanceUID.Text = "UID: " + m_source.GetSopInstanceUID();

            DrawSlices(0);
            DrawEcg(m_streamXY.GetFrameTimes()[0]);
        }

        private void DrawEcg (double cur_time)
        {
            EcgSeries ecg;
            try {
                ecg = m_source.GetECG();

                if (ecg.samples.Length == 0) {
                    ECG.Data = null; // ECG not available
                    return;
                }
            } catch (Exception) {
                ECG.Data = null; // ECG not available
                return;
            }

            // shrink width & height slightly, so that the "actual" width/height remain unchanged
            double W = (int)(ECG.ActualWidth - 1);
            double H = (int)(ECG.ActualHeight - 1);

            // horizontal scaling (index to X coord)
            double ecg_pitch = W/ecg.samples.Length;

            // vertical scaling (sample val to Y coord)
            double ecg_offset =  H*ecg.samples.Max()/(ecg.samples.Max()-ecg.samples.Min());
            double ecg_scale  = -H/(ecg.samples.Max()-ecg.samples.Min());

            // vertical scaling (time to Y coord conv)
            double time_offset = -W*ecg.start_time/(ecg.delta_time*ecg.samples.Length);
            double time_scale  =  W/(ecg.delta_time*ecg.samples.Length);

            PathGeometry pathGeom = new PathGeometry();
            {
                // draw ECG trace
                PathSegmentCollection pathSegmentCollection = new PathSegmentCollection();
                for (int i = 0; i < ecg.samples.Length; ++i) {
                    LineSegment lineSegment = new LineSegment();
                    lineSegment.Point = new Point(ecg_pitch*i, ecg_offset+ecg_scale*ecg.samples[i]);
                    pathSegmentCollection.Add(lineSegment);
                }

                PathFigure pathFig = new PathFigure();
                pathFig.StartPoint = new Point(0, ecg_offset+ecg_scale*ecg.samples[0]);
                pathFig.Segments = pathSegmentCollection;

                PathFigureCollection pathFigCol = new PathFigureCollection();
                pathFigCol.Add(pathFig);

                pathGeom.Figures = pathFigCol;
            }

            {
                // draw current frame line
                double x_pos = time_offset + time_scale * cur_time;

                LineGeometry line = new LineGeometry();
                line.StartPoint = new Point(x_pos, 0);
                line.EndPoint = new Point(x_pos, H);

                pathGeom.AddGeometry(line);
            }

            ECG.Stroke = Brushes.Blue;
            ECG.StrokeThickness = 1.0;
            ECG.Data = pathGeom;
        }

        private void FrameSelector_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var idx = (uint)FrameSelector.Value;
            DrawSlices(idx);
            DrawEcg(m_streamXY.GetFrameTimes()[idx]);
        }

        private void InitializeSlices()
        {
            Debug.Assert(m_source != null);

            uint stream_count = m_source.GetStreamCount();
            if (stream_count < 1)
                throw new Exception("No image streams found");

            Cart3dGeom bbox = m_source.GetBoundingBox();
            if (Math.Abs(bbox.dir3_y) > Math.Abs(bbox.dir2_y)) {
                // swap 2nd & 3rd axis, so that the 2nd becomes predominately "Y"
                SwapVals(ref bbox.dir2_x, ref bbox.dir3_x);
                SwapVals(ref bbox.dir2_y, ref bbox.dir3_y);
                SwapVals(ref bbox.dir2_z, ref bbox.dir3_z);
            }

            // extend bounding-box axes, so that dir1, dir2 & dir3 have equal length
            ExtendBoundingBox(ref bbox);

            const ushort HORIZONTAL_RES = 256;
            const ushort VERTICAL_RES = 256;

            // get XY plane (assumes 1st axis is "X" and 2nd is "Y")
            Cart3dGeom bboxXY = bbox;
            bboxXY.origin_x = bboxXY.origin_x + bboxXY.dir3_x / 2;
            bboxXY.origin_y = bboxXY.origin_y + bboxXY.dir3_y / 2;
            bboxXY.origin_z = bboxXY.origin_z + bboxXY.dir3_z / 2;
            bboxXY.dir3_x = 0;
            bboxXY.dir3_y = 0;
            bboxXY.dir3_z = 0;
            m_streamXY = m_source.GetStream(0, bboxXY, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });
            if (stream_count >= 2)
                m_streamXYcf = m_source.GetStream(1, bboxXY, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 }); // assume 2nd stream is color-flow

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
            m_streamXZ = m_source.GetStream(0, bboxXZ, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });
            if (stream_count >= 2)
                m_streamXZcf = m_source.GetStream(1, bboxXZ, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 }); // assume 2nd stream is color-flow

            // get ZY plane (assumes 2nd axis is "Y" and 3rd is "Z")
            Cart3dGeom bboxZY = bbox;
            bboxZY.origin_x = bbox.origin_x + bbox.dir1_x / 2;
            bboxZY.origin_y = bbox.origin_y + bbox.dir1_y / 2;
            bboxZY.origin_z = bbox.origin_z + bbox.dir1_z / 2;
            bboxZY.dir1_x = bbox.dir3_x;
            bboxZY.dir1_y = bbox.dir3_y;
            bboxZY.dir1_z = bbox.dir3_z;
            bboxZY.dir2_x = bbox.dir2_x;
            bboxZY.dir2_y = bbox.dir2_y;
            bboxZY.dir2_z = bbox.dir2_z;
            bboxZY.dir3_x = 0;
            bboxZY.dir3_y = 0;
            bboxZY.dir3_z = 0;
            m_streamZY = m_source.GetStream(0, bboxZY, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 });
            if (stream_count >= 2)
                m_streamZYcf = m_source.GetStream(1, bboxZY, new ushort[] { HORIZONTAL_RES, VERTICAL_RES, 1 }); // assume 2nd stream is color-flow
        }

        private void DrawSlices(uint frame)
        {
            Debug.Assert(m_source != null);

            ImageFormat image_format;
            byte[] tissue_map = m_source.GetColorMap(ColorMapType.TYPE_TISSUE_COLOR, out image_format);
            if (image_format != ImageFormat.IMAGE_FORMAT_R8G8B8A8)
                throw new Exception("Unexpected color-map format");

            byte[] cf_map = m_source.GetColorMap(ColorMapType.TYPE_FLOW_COLOR, out image_format);
            if (image_format != ImageFormat.IMAGE_FORMAT_R8G8B8A8)
                throw new Exception("Unexpected color-map format");

            byte[] arb_table = m_source.GetColorMap(ColorMapType.TYPE_FLOW_ARB, out image_format);
            if (image_format != ImageFormat.IMAGE_FORMAT_U8)
                throw new Exception("Unexpected color-map format");

            uint cf_frame = 0;
            if (m_streamXYcf != null) {
                // find closest corresponding CF frame
                double t_time = m_streamXY.GetFrame(frame).time;
                double[] cf_times = m_streamXYcf.GetFrameTimes();

                int closest_idx = 0;
                for (int i = 1; i < cf_times.Length; ++i) {
                    if (Math.Abs(cf_times[i] - t_time) < Math.Abs(cf_times[closest_idx] - t_time))
                        closest_idx = i;
                }

                cf_frame = (uint)closest_idx;
            }

            // get XY plane (assumes 1st axis is "X" and 2nd is "Y")
            Image3d imageXY = m_streamXY.GetFrame(frame);
            if (m_streamXYcf != null) {
                Image3d imageXYcf = m_streamXYcf.GetFrame(cf_frame);
                ImageXY.Source = GenerateBitmap(imageXY, imageXYcf, tissue_map, cf_map, arb_table);
            } else {
                ImageXY.Source = GenerateBitmap(imageXY, tissue_map);
            }
        
            // get XZ plane (assumes 1st axis is "X" and 3rd is "Z")
            Image3d imageXZ = m_streamXZ.GetFrame(frame);
            if (m_streamXZcf != null) {
                Image3d imageXZcf = m_streamXZcf.GetFrame(cf_frame);
                ImageXZ.Source = GenerateBitmap(imageXZ, imageXZcf, tissue_map, cf_map, arb_table);
            } else {
                ImageXZ.Source = GenerateBitmap(imageXZ, tissue_map);
            }

            // get ZY plane (assumes 2nd axis is "Y" and 3rd is "Z")
            Image3d imageZY = m_streamZY.GetFrame(frame);
            if (m_streamZYcf != null) {
                Image3d imageZYcf = m_streamZYcf.GetFrame(cf_frame);
                ImageZY.Source = GenerateBitmap(imageZY, imageZYcf, tissue_map, cf_map, arb_table);
            } else {
                ImageZY.Source = GenerateBitmap(imageZY, tissue_map);
            }

            FrameTime.Text = "Frame time: " + imageXY.time;
        }

        private WriteableBitmap GenerateBitmap(Image3d t_img, byte[] t_map)
        {
            Debug.Assert(t_img.format == ImageFormat.IMAGE_FORMAT_U8);

            WriteableBitmap bitmap = new WriteableBitmap(t_img.dims[0], t_img.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
            bitmap.Lock();
            unsafe {
                for (int y = 0; y < bitmap.Height; ++y) {
                    for (int x = 0; x < bitmap.Width; ++x) {
                        byte t_val = t_img.data[x + y * t_img.stride0];

                        // lookup tissue color
                        byte[] channels = BitConverter.GetBytes(BitConverter.ToUInt32(t_map, 4*t_val));

                        // assign red, green & blue
                        byte* pixel = (byte*)bitmap.BackBuffer + x * (bitmap.Format.BitsPerPixel / 8) + y * bitmap.BackBufferStride;
                        pixel[0] = channels[0]; // red
                        pixel[1] = channels[1]; // green
                        pixel[2] = channels[2]; // blue
                        // discard alpha channel
                    }
                }
            }
            bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
            bitmap.Unlock();
            return bitmap;
        }

        private WriteableBitmap GenerateBitmap(Image3d t_img, Image3d cf_img, byte[] t_map,  byte[] cf_map, byte[] arb_table)
        {
            Debug.Assert(t_img.dims.SequenceEqual(cf_img.dims));
            Debug.Assert(t_img.format == ImageFormat.IMAGE_FORMAT_U8);
            Debug.Assert(cf_img.format == ImageFormat.IMAGE_FORMAT_FREQ8POW8);

            WriteableBitmap bitmap = new WriteableBitmap(t_img.dims[0], t_img.dims[1], 96.0, 96.0, PixelFormats.Rgb24, null);
            bitmap.Lock();
            unsafe {
                for (int y = 0; y < bitmap.Height; ++y) {
                    for (int x = 0; x < bitmap.Width; ++x) {
                        byte t_val = t_img.data[x + y * t_img.stride0];
                        ushort cf_val = BitConverter.ToUInt16(cf_img.data, 2*x + y*(int)cf_img.stride0);

                        uint rgba = 0;
                        if (arb_table[cf_val] > t_val) {
                            // display color-flow overlay
                            rgba = BitConverter.ToUInt32(cf_map, 4*cf_val);
                        } else {
                            // display tissue underlay
                            rgba = BitConverter.ToUInt32(t_map, 4*t_val);
                        }

                        byte[] channels = BitConverter.GetBytes(rgba);

                        // assign red, green & blue
                        byte* pixel = (byte*)bitmap.BackBuffer + x * (bitmap.Format.BitsPerPixel / 8) + y * bitmap.BackBufferStride;
                        pixel[0] = channels[0]; // red
                        pixel[1] = channels[1]; // green
                        pixel[2] = channels[2]; // blue
                        // discard alpha channel
                    }
                }
            }
            bitmap.AddDirtyRect(new Int32Rect(0, 0, bitmap.PixelWidth, bitmap.PixelHeight));
            bitmap.Unlock();
            return bitmap;
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
