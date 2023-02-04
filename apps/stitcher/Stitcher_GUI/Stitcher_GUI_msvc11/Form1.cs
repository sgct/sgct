using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace Stitcher_GUI
{
    public partial class stitcher_form : Form
    {
        private OpenFileDialog openFileDialog;
        private FolderBrowserDialog browseFolderDialog;
        double eyeSeparation;
        double domeDiameter;
        
        public stitcher_form()
        {
            InitializeComponent();
            init();
        }

        public void init()
        {
            //set all comboboxes
            MSAA_ComboBox.SelectedIndex = 2;
            left_transform_comboBox.SelectedIndex = 0;
            right_transform_comboBox.SelectedIndex = 0;
            top_transform_comboBox.SelectedIndex = 0;
            bottom_transform_comboBox.SelectedIndex = 0;
            resolution_comboBox.SelectedIndex = 2;
            format_comboBox.SelectedIndex = 0;

            start_button.Enabled = false;

            updateStereoElements();

            // Create an instance of the open file dialog box.
            openFileDialog = new OpenFileDialog();

            // Set filter options and filter index.
            openFileDialog.Filter = "PNG images (.png)|*.png";
            openFileDialog.FilterIndex = 1;

            browseFolderDialog = new FolderBrowserDialog();

            eyeSeparation = 0.065;
            domeDiameter = 15.0;

            updateTextBoxes();
        }

        public void updateStereoElements()
        {
            if( !stereo_checkBox.Checked )
            {
                left_input_groupbox.Text = "Input";
                LeftOutputPathLabel.Text = "Path";

                right_input_groupbox.Enabled = false;
                
                //output items
                RightOutputPathLabel.Enabled = false;
                right_output_path_textbox.Enabled = false;
                browseRightOutputPathButton.Enabled = false;
            }
            else
            {
                left_input_groupbox.Text = "Left eye input";
                LeftOutputPathLabel.Text = "Left path";
                
                right_input_groupbox.Enabled = true;

                //output items
                RightOutputPathLabel.Enabled = true;
                right_output_path_textbox.Enabled = true;
                browseRightOutputPathButton.Enabled = true;
            }
        }

        private string checkIntegerInput(string str)
        {
            int tmpVal;
            string outStr = "";

            if (str.Length == 0)
                outStr = "0";
            else
            {
                if (Int32.TryParse(start_index_textBox.Text, out tmpVal))
                    outStr = tmpVal.ToString();
                else
                    outStr = "0";
            }

            return outStr;
        }

        private string checkDoubleInput(string str, string defaultVal, out double doubleOutput)
        {
            double tmpd = 0.0;
            string outStr = "";

            System.Globalization.NumberStyles style = System.Globalization.NumberStyles.AllowDecimalPoint;
            if (double.TryParse(str, style, System.Globalization.CultureInfo.InvariantCulture, out tmpd))
            {
                outStr = tmpd.ToString("0.00", System.Globalization.CultureInfo.InvariantCulture);
                doubleOutput = tmpd;
            }
            else
            {
                outStr = defaultVal;
                doubleOutput = double.Parse(defaultVal, style, System.Globalization.CultureInfo.InvariantCulture);
            }

            return outStr;
        }

        private void stereo_checkBox_CheckedChanged(object sender, EventArgs e)
        {
            updateStereoElements();
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void updateStartButtonStatus()
        {
            if( stereo_checkBox.Checked &&
                input_left_L_textbox.Text.Length > 4 &&
                input_right_L_textbox.Text.Length > 4 &&
                input_top_L_textbox.Text.Length > 4 &&
                input_bottom_L_textbox.Text.Length > 4 &&
                input_left_R_textbox.Text.Length > 4 &&
                input_right_R_textbox.Text.Length > 4 &&
                input_top_R_textbox.Text.Length > 4 &&
                input_bottom_R_textbox.Text.Length > 4 &&
                FileNameTextBox.Text.Length > 0 &&
                left_output_path_textbox.Text.Length > 0 &&
                right_output_path_textbox.Text.Length > 0)
            {
                start_button.Enabled = true;
            }
            else if ( !stereo_checkBox.Checked && 
                input_left_L_textbox.Text.Length > 4 &&
                input_right_L_textbox.Text.Length > 4 &&
                input_top_L_textbox.Text.Length > 4 &&
                input_bottom_L_textbox.Text.Length > 4 &&
                FileNameTextBox.Text.Length > 0 &&
                left_output_path_textbox.Text.Length > 0)
            {
                start_button.Enabled = true;
            }
            else
                start_button.Enabled = false;
        }

        private void updateTextBoxes()
        {
            if(  input_left_L_textbox.Text.Length > 4 )
                input_left_L_textbox.BackColor = SystemColors.Window;
            else
                input_left_L_textbox.BackColor = Color.Pink;

            if (input_right_L_textbox.Text.Length > 4)
                input_right_L_textbox.BackColor = SystemColors.Window;
            else
                input_right_L_textbox.BackColor = Color.Pink;

            if (input_top_L_textbox.Text.Length > 4)
                input_top_L_textbox.BackColor = SystemColors.Window;
            else
                input_top_L_textbox.BackColor = Color.Pink;

            if (input_bottom_L_textbox.Text.Length > 4)
                input_bottom_L_textbox.BackColor = SystemColors.Window;
            else
                input_bottom_L_textbox.BackColor = Color.Pink;
            
            if (stereo_checkBox.Checked)
            {
                if (input_left_R_textbox.Text.Length > 4)
                    input_left_R_textbox.BackColor = SystemColors.Window;
                else
                    input_left_R_textbox.BackColor = Color.Pink;

                if (input_right_R_textbox.Text.Length > 4)
                    input_right_R_textbox.BackColor = SystemColors.Window;
                else
                    input_right_R_textbox.BackColor = Color.Pink;

                if (input_top_R_textbox.Text.Length > 4)
                    input_top_R_textbox.BackColor = SystemColors.Window;
                else
                    input_top_R_textbox.BackColor = Color.Pink;

                if (input_bottom_R_textbox.Text.Length > 4)
                    input_bottom_R_textbox.BackColor = SystemColors.Window;
                else
                    input_bottom_R_textbox.BackColor = Color.Pink;

                if (right_output_path_textbox.Text.Length > 0)
                    right_output_path_textbox.BackColor = SystemColors.Window;
                else
                    right_output_path_textbox.BackColor = Color.Pink;
            }

            if (FileNameTextBox.Text.Length > 0)
                FileNameTextBox.BackColor = SystemColors.Window;
            else
                FileNameTextBox.BackColor = Color.Pink;

            if (left_output_path_textbox.Text.Length > 0)
                left_output_path_textbox.BackColor = SystemColors.Window;
            else
                left_output_path_textbox.BackColor = Color.Pink;
        }

        private void browse_left_L_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_left_L_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_right_L_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_right_L_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_top_L_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_top_L_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_bottom_L_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_bottom_L_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_left_R_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_left_R_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_right_R_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_right_R_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_top_R_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_top_R_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browse_bottom_R_button_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() == DialogResult.OK && openFileDialog.FileName.Length > 0)
            {
                input_bottom_R_textbox.Text = openFileDialog.FileName;
                updateStartButtonStatus();
            }
        }

        private void browseLeftOutputPathButton_Click(object sender, EventArgs e)
        {
            if( browseFolderDialog.ShowDialog() == DialogResult.OK && browseFolderDialog.SelectedPath.Length > 0 )
            {
                left_output_path_textbox.Text = browseFolderDialog.SelectedPath;
                updateStartButtonStatus();
            }
        }

        private void browseRightOutputPathButton_Click(object sender, EventArgs e)
        {
            if (browseFolderDialog.ShowDialog() == DialogResult.OK && browseFolderDialog.SelectedPath.Length > 0)
            {
                right_output_path_textbox.Text = browseFolderDialog.SelectedPath;
                updateStartButtonStatus();
            }
        }

        private void input_left_L_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_right_L_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_top_L_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_bottom_L_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_left_R_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_right_R_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_top_R_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void input_bottom_R_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void start_button_Click(object sender, EventArgs e)
        {
			string processName = "stitcher";
            string arguments = "-config fisheye.xml";

            if( stereo_checkBox.Checked )
            {
                arguments += " -tex \"" + input_left_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_right_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_top_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_bottom_L_textbox.Text + "\"";

                arguments += " -tex \"" + input_left_R_textbox.Text + "\"";
                arguments += " -tex \"" + input_right_R_textbox.Text + "\"";
                arguments += " -tex \"" + input_top_R_textbox.Text + "\"";
                arguments += " -tex \"" + input_bottom_R_textbox.Text + "\"";

				arguments += " -leftPath \"" + left_output_path_textbox.Text + "/" + FileNameTextBox.Text + "\"";
				arguments += " -rightPath \"" + right_output_path_textbox.Text + "/" + FileNameTextBox.Text + "\"";
            }
            else
            {
                arguments += " -tex \"" + input_left_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_right_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_top_L_textbox.Text + "\"";
                arguments += " -tex \"" + input_bottom_L_textbox.Text + "\"";
                arguments += " -leftPath \"" + left_output_path_textbox.Text + "\\" + FileNameTextBox.Text + "\"";
            }

            arguments += " -start " + start_index_textBox.Text;
            arguments += " -seq " + input_startindex_textBox.Text + " " + input_stopindex_textBox.Text;
            arguments += " -rot " + left_transform_comboBox.Items[left_transform_comboBox.SelectedIndex].ToString();
            arguments += " " + right_transform_comboBox.Items[right_transform_comboBox.SelectedIndex].ToString();
            arguments += " " + top_transform_comboBox.Items[top_transform_comboBox.SelectedIndex].ToString();
            arguments += " " + bottom_transform_comboBox.Items[bottom_transform_comboBox.SelectedIndex].ToString();

            if (alpha_checkBox.Checked)
                arguments += " -alpha 1";
            else
                arguments += " -alpha 0";

            if (stereo_checkBox.Checked)
                arguments += " -stereo 1";
            else
                arguments += " -stereo 0";

            if (FXAACheckBox.Checked)
                arguments += " -fxaa 1";
            else
                arguments += " -fxaa 0";


            arguments += " -eyeSep " + eyeSeparation.ToString("0.000000", System.Globalization.CultureInfo.InvariantCulture);
            arguments += " -diameter " + domeDiameter.ToString("0.000000", System.Globalization.CultureInfo.InvariantCulture);
            arguments += " -msaa " + MSAA_ComboBox.Items[MSAA_ComboBox.SelectedIndex].ToString();
            
            string resolutionString = resolution_comboBox.Items[resolution_comboBox.SelectedIndex].ToString();
            arguments += " -res " + resolutionString.Substring(0, resolutionString.IndexOf("x"));

            arguments += " -format " + format_comboBox.Items[format_comboBox.SelectedIndex].ToString();
            arguments += " -compression " + compressionTrackBar.Value.ToString();

            //MessageBox.Show(arguments);

            ProcessStartInfo info = new ProcessStartInfo(processName, arguments);
            info.UseShellExecute = false;
            info.WorkingDirectory = Directory.GetCurrentDirectory();

            try
            {
                Process.Start(info);
            }
            catch
            {
                MessageBox.Show("Couldn't find stitcher_msvc12_x64.exe!");
            }
        }

        private void start_index_textBox_TextChanged(object sender, EventArgs e)
        {
            start_index_textBox.Text = checkIntegerInput(start_index_textBox.Text);
        }

        private void input_stopindex_textBox_TextChanged(object sender, EventArgs e)
        {
            input_stopindex_textBox.Text = checkIntegerInput(input_stopindex_textBox.Text);
        }

        private void input_startindex_textBox_TextChanged(object sender, EventArgs e)
        {
            input_startindex_textBox.Text = checkIntegerInput(input_startindex_textBox.Text);
        }

        private void eyeSeparationInput_TextChanged(object sender, EventArgs e)
        {
            eyeSeparationInput.Text = checkDoubleInput(eyeSeparationInput.Text, "65.00", out eyeSeparation);
            eyeSeparation /= 1000.0;
        }

        private void domeDiameterInput_TextChanged(object sender, EventArgs e)
        {
            domeDiameterInput.Text = checkDoubleInput(domeDiameterInput.Text, "15.00", out domeDiameter);
        }

        private void format_comboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if( format_comboBox.SelectedIndex == 0 ) //if PNG
            {
                CompressionLabel.Enabled = true;
                compressionTrackBar.Enabled = true;
            }
            else
            {
                CompressionLabel.Enabled = false;
                compressionTrackBar.Enabled = false;
            }
        }

        private void right_output_path_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void left_output_path_textbox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }

        private void FileNameTextBox_TextChanged(object sender, EventArgs e)
        {
            updateStartButtonStatus();
            updateTextBoxes();
        }
    }
}
