using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Diagnostics;
using System.Windows.Controls.Primitives;
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
        }
    }
}
