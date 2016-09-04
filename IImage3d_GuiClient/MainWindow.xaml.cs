using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;
using Image3dAPI;
using System.IO;
using System.Xml;
using System.Windows.Controls.Primitives;

namespace IImage3d_GuiClient
{
    public partial class MainWindow : Window
    {
              
//      private variables
        private int x;              //current x,y,z pixel value
        private int y;              
        private int z;              
        private float x_bound;      //Real space image measurement in x,y,z dimension
        private float y_bound;      
        private float z_bound;     
        private float x_Length;     //Adjusted image dimensions based on real space bounds
        private float y_Length;
        private float z_Length;
        private ushort[] dimValues;         //Pixel dimensions of image. Array contains 3 values for x,y,z
        private int currentFrame = 1;   
        private double frameTime;
        private double frameRate = 60;
        private uint frameCount;
        private double duration;            //Duration of frame slider animation
        private double[] frameTimesArray;   //Array of all the individual frame times
        private byte[][] allDataArray;      //2D array for all image data. 1st dim is frame, 2nd is frameData arrays
        private byte[] frameDataArray;      //byte array of pixel image data for a frame
        private int maxSliderLength = 750;  //750 slider is equivilent to 300 image width
        private int maxImageWidth = 300;    //Based on screen size
        private int stride0;
        private int stride1;
        private System.Windows.Controls.Image xImage;   //Current xplane, yplane, zplane images
        private System.Windows.Controls.Image yImage;
        private System.Windows.Controls.Image zImage;
        private uint[] colorMap = new uint[256];
        private string progID = "some id";

       //Image Information Item Descriptions
        public class imageInfoItem
        {
            public string description { get; set; }
        }

        // COM DLL setup
        // All COM DLLs must export the DllRegisterServer()
        // and the DllUnregisterServer() APIs for self-registration/unregistration.
        // They both have the same signature and so only one
        // delegate is required.
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate UInt32 DllRegUnRegAPI();

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr LoadLibrary([MarshalAs(UnmanagedType.LPStr)]string strLibraryName);

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        static extern Int32 FreeLibrary(IntPtr hModule);

        [DllImport("Kernel32.dll", CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr GetProcAddress(IntPtr hModule, [MarshalAs(UnmanagedType.LPStr)] string lpProcName);

        /******************************************************************************
                                 * Initial Setup *
        *******************************************************************************/
        //Loads Dll and image files
        //Reads interface and file data,
        //Displays the data that does not change with sliders       
        public MainWindow()
        {
            InitializeComponent();

            // Set initial name and placement of load file grid
            loadGridLabel.Content = "IImage3d Image Viewer";


            //If xml file with current dll information exists, load the info
            XmlDocument dllInfoXml = new XmlDocument();
            if(File.Exists(Environment.CurrentDirectory + "//IImage3dDllInfo.xml"))           
            {
                try
                {
                    dllInfoXml.Load(Environment.CurrentDirectory + "//IImage3dDllInfo.xml");
                    ProgIdTextBox.Text = dllInfoXml.DocumentElement.SelectSingleNode("/dllImageInfo/progID").InnerText;
                    ImageFileTextBox.Text = dllInfoXml.DocumentElement.SelectSingleNode("/dllImageInfo/dcmFile").InnerText;
                    //If info exists, automatically push the load image buton
                    LoadImageButton.RaiseEvent(new RoutedEventArgs(ButtonBase.ClickEvent));
                }
                catch
                {
                    //File did not load correctly. Continue as if no file existed.
                    ProgIdTextBox.Text = "";
                }
                
            }           
        }
        
        //Load Image Button
        //After user clicks this button, the program attempts to load the dll
        //and image files. If successfully loaded, the inital image information
        //and plane images are displayed.
        private void LoadImageButton_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            try
            {
                IImage3dSource source = null;
                IImage3dFileLoader loader = null;

                //CoCreateInstance of loaded DLL
                progID = ProgIdTextBox.Text;
                try
                {
                    Type comType = Type.GetTypeFromProgID(ProgIdTextBox.Text);
                    if (comType == null)
                    {
                        System.Windows.MessageBox.Show("Could not create instance of IImage3dFileLoader. Check progid and DLL.\n");
                        return;
                    }
                    loader = (IImage3dFileLoader)Activator.CreateInstance(comType);
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show("Could not create instance of IImage3dFileLoader. Check progid and DLL.\n\nError Details\n" + ex.ToString());
                    return;
                }

                //load image file
                //If we've made it this far, the dll is successfully loaded
                string imageLoadError = null;
                if (ImageFileTextBox.Text != "")
                    imageLoadError = loader.LoadFile(ImageFileTextBox.Text);
                //Create Image3dSource object and cast to COM interface 
                source = loader.GetImageSource();
                if (imageLoadError != null || source == null)
                {
                    System.Windows.MessageBox.Show("Invalid image: Could not load image file");
                    return;
                }

                //Create XML file with dll and progid info
                string dllImageInfo = "<?xml version=\"1.0\"?>\n<dllImageInfo>\n\t<progID>" + progID + "</progID>\n\t<dcmFile>" + ImageFileTextBox.Text + "</dcmFile>\n" +
                    "</dllImageInfo>\n";
                try
                {
                    File.WriteAllText(Environment.CurrentDirectory + "//IImage3dDllInfo.xml", dllImageInfo);
                }
                catch (Exception ex) //Even if file writing fails, continue with program.
                {
                    System.Windows.MessageBox.Show("Could not save dll information to file. \n\nError Details\n" + ex.ToString());
                }

                //If image is successfully loaded, move the load file text boxes
                Grid.SetRow(loadGrid, 1);
                Grid.SetColumn(loadGrid, 2);
                loadGridLabel.Content = "Load Image File / DLL";


                //get frame information
                frameCount = source.GetFrameCount();    //get frame count
                if (frameCount < 1)
                {
                    System.Windows.MessageBox.Show("Frame count < 1. Cannot display image");
                    return;
                }
                frameTimesArray = new double[frameCount];
                frameTimesArray = source.GetFrameTimes(); //get frame times 
                frameTime = frameTimesArray[0];           //get initial frame time
                if (frameCount > 1 && frameTimesArray.Length > 1)
                {
                    if (frameTimesArray[1] - frameTimesArray[0] > 0)
                    {
                        frameRate = 1 / (frameTimesArray[1] - frameTimesArray[0]); //get frameRate using difference between first two frames
                        duration = ((frameTimesArray[1] - frameTimesArray[0]) * frameCount);  //get animation duration using difference between first two frames and total frames                                                                     //           this.durationProperty = new TimeSpan(0, 0, Convert.ToInt32(duration)).ToString("c");
                        frameAnimation.Duration = new TimeSpan(0, 0, Convert.ToInt32((1 / frameRate * frameCount)));
                        frameAnimation.To = frameCount - 1;
                        frameSlider.Maximum = frameCount - 1;
                        FrameTextBox.Text = Convert.ToInt32(frameRate).ToString();
                    }
                    else
                    {
                        System.Windows.MessageBox.Show("Could not get frame interval.\nFrame interval <= 0.\nOnly first frame will be displayed)");
                    }
                }
                else //If there is only one frame, hide the frame slider behind a black canvas
                {
                    Canvas.SetZIndex(noFrameCanvas, (int)99);
                }
                //get real space geometry - Cart3dGeom
                Image3dAPI.Cart3dGeom geom = source.GetBoundingBox();
                float x_origin = geom.origin_x; //Get the origin displacement
                float y_origin = geom.origin_y;
                float z_origin = geom.origin_z;
                x_bound = geom.dir1_x;          //Get the real space bounds
                y_bound = geom.dir2_y;
                z_bound = geom.dir3_z;
                if (x_bound <= 0 || y_bound <= 0 || z_bound <= 0) //check for invalid dimensions
                {
                    System.Windows.MessageBox.Show("Invalid Image Dimensions\nOne of the following is <=0\ngeom.dir1_x\ngeom.dir1_y\ngeom.dir1_z");
                    return;
                }

                //get image data for first frame. 
                uint frame = 0;
                ushort[] max_res = { 150, 150, 150 };
                Image3dAPI.Image3d imageData = source.GetFrame(frame, geom, max_res);

                //get pixel dimensions, stride lengths, and adjusted image dimensions based on real space.
                dimValues = imageData.dims;
                stride0 = imageData.stride0;
                stride1 = imageData.stride1;
                //Verify that all dims and stride are greater than 1.
                if (dimValues[0] <= 1 || dimValues[1] <= 1 || dimValues[2] <= 1 || stride0 <= 1 || stride1 <= 1)
                {
                    System.Windows.MessageBox.Show("Invalid Image Dimensions\nOne of the following is <=1\nimageData.dims[0]\nimageData.dims[1]\nimageData.dims[2]\nstride0\nstride1");
                    return;
                }
                //Verify that strides are longer than corresponding dimensions
                if (stride0 < dimValues[0] || stride1 < dimValues[0] * dimValues[1])
                {
                    System.Windows.MessageBox.Show("Invaled stride dimensions. Either stride0 < x or stride1 < x*y");
                    return;
                }
                //Find largest bound and set that dimension to the maxImageWidth.
                //Use this ratio to determine the other image dimsions.
                //Set Slider lengths to match the image dimensions.
                //x_bound, y_bound, and z_bound have already been verified to be > 0.
                if (x_bound > y_bound)
                {
                    if (x_bound > z_bound)
                    {
                        x_Length = maxImageWidth;
                        y_Length = maxImageWidth * y_bound / x_bound;
                        z_Length = maxImageWidth * z_bound / x_bound;
                        xSlider.Width = xSlider2.Width = maxSliderLength;
                        ySlider.Width = ySlider2.Height = (int)(maxSliderLength * y_bound / x_bound);
                        zSlider.Height = zSlider2.Height = (int)(maxSliderLength * z_bound / x_bound);

                    }
                    else
                    {
                        z_Length = maxImageWidth;
                        y_Length = maxImageWidth * y_bound / z_bound;
                        x_Length = maxImageWidth * x_bound / z_bound;
                        zSlider.Height = zSlider2.Height = maxSliderLength;
                        ySlider.Width = ySlider2.Height = (int)(maxSliderLength * y_bound / z_bound);
                        xSlider.Width = xSlider2.Width = (int)(maxSliderLength * x_bound / z_bound);

                    }
                }
                else if (y_bound > z_bound)
                {
                    y_Length = maxImageWidth;
                    z_Length = maxImageWidth * z_bound / y_bound;
                    x_Length = maxImageWidth * x_bound / y_bound;
                    ySlider.Width = ySlider2.Height = maxSliderLength;
                    zSlider.Height = zSlider2.Height = (int)(maxSliderLength * z_bound / y_bound);
                    xSlider.Width = xSlider2.Width = (int)(maxSliderLength * x_bound / y_bound);

                }
                else
                {
                    z_Length = maxImageWidth;
                    y_Length = maxImageWidth * y_bound / z_bound;
                    x_Length = maxImageWidth * x_bound / z_bound;
                    zSlider.Height = zSlider2.Height = maxSliderLength;
                    ySlider.Width = ySlider2.Height = (int)(maxSliderLength * y_bound / z_bound);
                    xSlider.Width = xSlider2.Width = (int)(maxSliderLength * x_bound / z_bound);
                }            

                //Create an array of byte arrays, one for each frame, stored in allDataArray
                allDataArray = new byte[frameCount][];
                for (uint i = 0; i < frameCount; i++)
                {
                    Image3dAPI.Image3d im3dData = source.GetFrame(i, geom, max_res);
                    allDataArray[i] = im3dData.data;
                }
                frameDataArray = allDataArray[0];

                //get ColorMap
                colorMap = source.GetColorMap();

                //get ECG info
                Image3dAPI.EcgSeries ecg = source.GetECG();
                double startTime = ecg.start_time;

                //get Probe info
                Image3dAPI.ProbeInfo probe = source.GetProbeInfo();
                string pName = probe.name;
                Image3dAPI.ProbeType pType = probe.type;

                //get Sop instance 
                string sop = "Some UID goes here";

                //Display image information that will not change with frame or dimension sliders.
                List<imageInfoItem> imageInfoCategory = new List<imageInfoItem>();
                imageInfoCategory.Add(new imageInfoItem() { description = "SOP instance UID:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Probe Name:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Probe Type:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Image Start Time:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Image Format:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Bytes per pixel:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Image resolution:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Dispacement from probe tip:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "Total size of image:" });
                imageInfoCategory.Add(new imageInfoItem() { description = "" });

                imageInfoCategoryItems.ItemsSource = imageInfoCategory;

                List<imageInfoItem> imageInfoValue = new List<imageInfoItem>();
                imageInfoValue.Add(new imageInfoItem() { description = "" + sop });
                imageInfoValue.Add(new imageInfoItem() { description = "" + pName });
                imageInfoValue.Add(new imageInfoItem() { description = "" + pType });
                imageInfoValue.Add(new imageInfoItem() { description = "" + imageData.time });
                imageInfoValue.Add(new imageInfoItem() { description = "" + imageData.format });
                imageInfoValue.Add(new imageInfoItem() { description = "1" });
                imageInfoValue.Add(new imageInfoItem() { description = "" + imageData.dims[0] + " x " + imageData.dims[1] + " x " + imageData.dims[2] + " pixels" });
                imageInfoValue.Add(new imageInfoItem() { description = "" + "x = " + x_origin + ", y = " + y_origin + ", z= " + z_origin + " (m)" });
                imageInfoValue.Add(new imageInfoItem() { description = "" + "x = " + x_bound + ", y = " + y_bound + ", z= " + z_bound + " (m)" });

                imageInfoValueItems.ItemsSource = imageInfoValue;

                /*           
                //Gradient test - Dummy data
                                   int dimX = 600;
                                   int dimY = 600;
                                   int dimZ = 600;
                                   stride0 = dimX;
                                   stride1 = dimX*dimY;
                                   byte[] gradient = createGradient(dimX, dimY, dimZ);
                                   byte[] white = createWhite(dimX, dimY, dimZ);
                                   for (int i = 0; i < 59; i=i+2)
                                   {
                                       allDataArray[i] = gradient;
                                       allDataArray[i + 1] = white;
                                   }
                                   frameDataArray = allDataArray[0];

                                   dimValues[0] = (ushort)(dimX);  // in this case 6
                                   dimValues[1] = (ushort)(dimY);  // in this case 7
                                   dimValues[2] = (ushort)(dimZ);  // in this case 8 

                 */

                //Set dimension data for use in XAML with max Slider values
                xSlider.Maximum = xSlider2.Maximum = dimValues[0] - 1;
                ySlider.Maximum = ySlider2.Maximum = dimValues[1] - 1;
                zSlider.Maximum = zSlider2.Maximum = dimValues[2] - 1;
                xSlider.Value = xSlider2.Value = dimValues[0] / 2;
                ySlider.Value = ySlider2.Value = dimValues[1] / 2;
                zSlider.Value = zSlider2.Value = dimValues[2] / 2;

                displayDynamicImageInfo();
                displayImageX();    //Call displayImage to display the the rest of the info that changes with frame/sliders
                displayImageY();    
                displayImageZ();    
            }
            catch (Exception exp)
            {
                System.Windows.MessageBox.Show("Could not load image.\n\nError Details\n" + exp.ToString());
                return;
            }

        }
        
        
        /******************************************************************************
                             * Create and Display images *
        *******************************************************************************/
        //Creates and displays current images based on dimension and frame sliders
        //Updates current statistics 


        //Displays information and images based on the current dimension and frame sliders.
        private void displayDynamicImageInfo()
        {
            //Display info that chanes with dimension and frame sliders
            //x_bound, y_bound, and z_bound have already been verified to be > 0.

            List<imageInfoItem> imageDimensionCategory = new List<imageInfoItem>();
            imageDimensionCategory.Add(new imageInfoItem() { description = "Pixel shift: " });
            imageDimensionCategory.Add(new imageInfoItem() { description = "" });
            imageDimensionCategory.Add(new imageInfoItem() { description = "" });
            imageDimensionCategory.Add(new imageInfoItem() { description = "Distance shift (m from origin): " });
            imageDimensionCategory.Add(new imageInfoItem() { description = "" });
            imageDimensionCategory.Add(new imageInfoItem() { description = "" });
            imageDimensionCategory.Add(new imageInfoItem() { description = "Current Frame = " });
            imageDimensionCategory.Add(new imageInfoItem() { description = "Current Frame Time = " });
//For testing purposes:            imageDimensionCategory.Add(new imageInfoItem() { description = "duration = " + this.durationProperty });
            imageDimensionCategory.Add(new imageInfoItem() { description = "" });

            dimensionCategoryItems.ItemsSource = imageDimensionCategory;

            //All dimValues have been verified to be > 1
            List<imageInfoItem> imageDimensionValue = new List<imageInfoItem>();
            imageDimensionValue.Add(new imageInfoItem() { description = "" });
            imageDimensionValue.Add(new imageInfoItem() { description = "x = " + x });
            imageDimensionValue.Add(new imageInfoItem() { description = "y = " + y });
            imageDimensionValue.Add(new imageInfoItem() { description = "z = " + z });
            imageDimensionValue.Add(new imageInfoItem() { description = "x = " + (x_bound * x / (dimValues[0]-1)) });
            imageDimensionValue.Add(new imageInfoItem() { description = "y = " + (y_bound * y / (dimValues[1]-1)) });
            imageDimensionValue.Add(new imageInfoItem() { description = "z = " + (z_bound * z / (dimValues[2]-1)) });
            imageDimensionValue.Add(new imageInfoItem() { description = "" + currentFrame });
            imageDimensionValue.Add(new imageInfoItem() { description = "" + frameTime });

            dimensionValueItems.ItemsSource = imageDimensionValue;
        }

        // ----------------------------------------------------------------------------------------
        // Image data

        //*** z plane ***************************************           

        private void displayImageZ()
        {
            //z plane dimensions
            int width = dimValues[0]; // x dimension
            int height = dimValues[1]; // y dimension
            int stride = stride0;   //stride0 is passed on to BitmapSource.Create later on

            //Create data array for single z plane
            byte[] zImageData = new byte[stride0 * height];
            Array.Copy(frameDataArray, z * stride1, zImageData, 0, stride0 * height);      //Fill array with data at plane 'z'.

            // Create bitmap and add to display
            zplane.Children.Remove(zImage);       //remove previous image    
            zImage = createImage(width, height, stride0, zImageData);
            zImage.Width = x_Length;            //stretch image to fit real space ratio
            zImage.Height = y_Length;
            zImage.Stretch = Stretch.Fill;

            //Flip image for correct display orientation
            zImage.RenderTransformOrigin = new System.Windows.Point(0.5, 0.5);
            ScaleTransform flipTrans = new ScaleTransform();
            flipTrans.ScaleY = -1;
            zImage.RenderTransform = flipTrans;

            RenderOptions.SetBitmapScalingMode(zImage, BitmapScalingMode.NearestNeighbor); //for sharp pixel boundaries
            zplane.Children.Add(zImage);    //add image to zplane grid

        }


        //*** y plane **********************************************                  
        private void displayImageY()
        //y plane dimensions
        {
            int width = dimValues[0]; // x dimension
            int height = dimValues[2]; // z dimension
            int depth = dimValues[1]; // y dimension
            int stride = width; //Stride is taken into account during array copy and therefore equals width in created array

            //Create data array for single y plane
            byte[] yImageData = new byte[width * height];
            for (int i = 0; i < height; i++) //Fill array with data at plane 'y'.
            {
                Array.Copy(frameDataArray, i * stride1 + stride0 * y, yImageData, i * width, width);
            }

            // Create bitmap and add to display
            yplane.Children.Remove(yImage);       //remove previous image    
            yImage = createImage(width, height, stride, yImageData);
            yImage.Width = x_Length;            //stretch image to fit real space ratio
            yImage.Height = z_Length;
            yImage.Stretch = Stretch.Fill;
            RenderOptions.SetBitmapScalingMode(yImage, BitmapScalingMode.NearestNeighbor); //for sharp pixel boundaries
            yplane.Children.Add(yImage);    //add image to zplane grid

        }

        //*** x plane **********************************************           
        private void displayImageX()
        {
            //x plane dimensions
            int width = dimValues[1]; // y dimension
            int height = dimValues[2]; // z dimension
            int depth = dimValues[0]; // x dimension
            int stride = width;

            //Create data array for single y plane
            byte[] xImageData = new byte[width * height];
            for (int i = 0; i < height; i++)        //Fill array with data at plane 'y'.
            {
                for(int j = 0; j < width; j++)
                {
                    xImageData[width * i + j] = frameDataArray[j * stride0 + i * stride1 + x];
                }
            }

            // Create bitmap and add to display
            xplane.Children.Remove(xImage);       //remove previous image               
            xImage = createImage(width, height, stride, xImageData);
            xImage.Width = y_Length;            //stretch image to fit real space ratio
            xImage.Height = z_Length;
            xImage.Stretch = Stretch.Fill;
            RenderOptions.SetBitmapScalingMode(xImage, BitmapScalingMode.NearestNeighbor); //for sharp pixel boundaries
            xplane.Children.Add(xImage);     //add image to zplane grid           
        }


        //**** CreateImage Subroutine******
        //  Parameteres:
        //  Width, height, and stride: pixel measurements
        //  Plane: plane pixel number of plane being desplayed
        //  imData: the two dimensional byte array image data
        public System.Windows.Controls.Image createImage(int width, int height, int stride, byte[] imData)
        {
            //General bitmap data            
            double dpiX = 96;
            double dpiY = 96;

            BitmapSource bitmap;

            if (colorMap.Length != 256)
            {
                var pixelFormat = PixelFormats.Gray8; // 8 bit grayscale. 1 byte per pixel
                bitmap = BitmapSource.Create(width, height, dpiX, dpiY, pixelFormat, null, imData, stride);
            }
            else
            {
                //Implement Color map
                var pixelFormat = PixelFormats.Bgra32; // 32 bit color. 4 bytes per pixel

                //Create Bitmap
                byte[] imDataColor = new byte[imData.Length * 4];

                //Apply color map
                int counter = 0;
                for (int i = 0; i < imData.Length; i++)
                {
                    uint k = colorMap[imData[i]];
                    int rem = 0;
                    byte A = (byte)(k / (Math.Pow(2, 24)));
                    rem = (int)(k % (Math.Pow(2, 24)));
                    byte B = (byte)(rem / (Math.Pow(2, 16)));
                    rem = (int)(k % (Math.Pow(2, 16)));
                    byte G = (byte)(rem / (Math.Pow(2, 8)));
                    rem = (int)(k % (Math.Pow(2, 8)));
                    byte R = (byte)rem;
                    imDataColor[counter] = R;
                    imDataColor[counter + 1] = G;
                    imDataColor[counter + 2] = B;
                    imDataColor[counter + 3] = A;
                    counter += 4;
                }

                bitmap = BitmapSource.Create(width, height, dpiX, dpiY, pixelFormat, null, imDataColor, stride * 4);
            }
             
            // Create Image and set its width and height  
            System.Windows.Controls.Image image = new System.Windows.Controls.Image();
  
            image.Source = bitmap;
                                  
            return image;
        }


        /******************************************************************************
                                     * User Controls *
        *******************************************************************************/
        //Methods that respond to user controls (sliders, textbox, button)

        //Image file browse button
        private void imageFileBrowseButton_Click(object sender, RoutedEventArgs e)
        {
            // Create OpenFileDialog 
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();

            // Set filter for file extension and default file extension 
            dlg.DefaultExt = ".dcm";
            dlg.Filter = "DICOM Files (*.dcm)|*.dcm|All Files (*.*)|*.*";

            // Display OpenFileDialog by calling ShowDialog method 
            bool? result = dlg.ShowDialog();

            // Get the selected file name and display in a TextBox 
            if (result == true)
            {
                // Open document 
                string filename = dlg.FileName;
                ImageFileTextBox.Text = filename;
            }
        }

        //xSlider, ySlider, zSlider
        //Sets the current plane value based on slider position and re-displays the image
        private void xSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            //Get Slider reference and get value
            var slider = sender as Slider;
            int value = (int)slider.Value;
            //Set variable and re-display images with new data.
            x = value;
            xSlider.Value = xSlider2.Value = value;
            displayDynamicImageInfo();
            displayImageX();
        }

        private void ySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            //Get Slider reference and get value
            var slider = sender as Slider;
            int value = (int)slider.Value;
            //Set variable and re-display images with new data.
            y = value;
            ySlider.Value = ySlider2.Value = value;
            displayDynamicImageInfo();
            displayImageY();
        }
        private void zSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            //Get Slider reference and get value
            var slider = sender as Slider;
            int value = (int)slider.Value;
            //Set variable and re-display images with new data.
            z = value;
            zSlider.Value = zSlider2.Value = value;
            displayDynamicImageInfo();
            displayImageZ();
        }

        //frameSlider
        //Sets the current frame value based on slider position and re-displays all three
        private void frameSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            //Get Slider reference and get value
            var slider = sender as Slider;
            int value = (int)slider.Value;
            //Set variables and re-display images with new data.
            frameDataArray = allDataArray[value]; //get the current frame data
            frameTime = frameTimesArray[value];   //get the current frame time
            currentFrame = value + 1;
            displayDynamicImageInfo();
            displayImageX();
            displayImageY();
            displayImageZ();
        }

        //Makes sure that input for frame rate is a positive integer.
        private void NumberValidationTextBox(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex("[^0-9]+");
            e.Handled = regex.IsMatch(e.Text);
        }

        //FrameTextBox
        //Set a new frame rate based on user input
        private void FrameTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            double oldFrameRate = frameRate;
            var textBox = sender as System.Windows.Controls.TextBox; //Get textbox reference
            double.TryParse(textBox.Text, out frameRate);   //convert text to double
            if (frameRate < 71 && frameRate >= 0)  //only allow values from 1 to 70
            {
                FrameTextBox.Text = textBox.Text;
                if (frameRate == 0)
                    frameRate = oldFrameRate;
                duration = (1 / frameRate * frameCount); //frameRate already determined as != 0
                frameAnimation.Duration = new TimeSpan(0, 0, Convert.ToInt32((1 / frameRate * frameCount)));                
            }
            else
            {
                System.Windows.MessageBox.Show("Please input 1 to 70 only.");
            }
           
        }        

       

/******************************************************************************
                            * Gradient Test *
*******************************************************************************/

        //Temporary way to test image viewer with data. Locally created.       
        //Create gradient with values from 0 to 85 for each dimensions so total range is 0 to 255
        public byte[] createGradient(double a, double b, double c)
        {
            double pixel = 255 / (a + b + c - 3);
            int count = 0;
            byte[] gradient = new byte[(int)a * (int)b * (int)c];             
            for (int z = 0; z < c; ++z)
            {
                for (int y = 0; y < b; ++y)
                {
                    for (int x = 0; x < a; ++x)
                    {
                        gradient[count] = (byte)(pixel * (x + y + z));
                        count++;
                    }
                }
            }
            return gradient;
        }
        public byte[] createWhite(int a, int b, int c)
        {
            int pixel = 255 / (a + b + c - 3);
            int count = 0;
            byte[] white = new byte[a * b * c];
            for (int z = 0; z < c; ++z)
            {
                for (int y = 0; y < b; ++y)
                {
                    for (int x = 0; x < a; ++x)
                    {
                        white[count] = 255;
                        count++;
                    }
                }
            }
            return white;
        }
    }
}
