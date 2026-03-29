using System;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;

namespace AgeCalculatorApp
{
    public partial class AgeCalculatorForm : Form
    {
        private Label label;
        private TextBox ageTextBox;
        private Button submitButton;

        public AgeCalculatorForm()
        {
            // 启用高DPI缩放
            this.AutoScaleDimensions = new SizeF(96f, 96f);
            this.AutoScaleMode = AutoScaleMode.Dpi;
            
            // 设置窗体属性
            this.Text = "年龄计算器";
            this.Size = new Size(400, 200);
            this.StartPosition = FormStartPosition.CenterScreen;
            
            InitializeComponents();
        }

        private void InitializeComponents()
        {
            // 创建标签
            label = new Label();
            label.Text = "请输入您的年龄：";
            label.Location = new Point(20, 30);
            label.Size = new Size(180, 25);
            this.Controls.Add(label);

            // 创建文本框
            ageTextBox = new TextBox();
            ageTextBox.Location = new Point(20, 60);
            ageTextBox.Size = new Size(150, 25);
            this.Controls.Add(ageTextBox);

            // 创建提交按钮
            submitButton = new Button();
            submitButton.Text = "提交";
            submitButton.Location = new Point(20, 100);
            submitButton.Size = new Size(80, 30);
            submitButton.Click += SubmitButton_Click;
            this.Controls.Add(submitButton);
        }

        private void SubmitButton_Click(object sender, EventArgs e)
        {
            // 验证用户是否输入了年龄
            if (string.IsNullOrWhiteSpace(ageTextBox.Text))
            {
                MessageBox.Show("请先输入您的年龄！", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // 检查输入是否为有效数字
            if (!int.TryParse(ageTextBox.Text, out _))
            {
                MessageBox.Show("请输入有效的年龄数字！", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // 模拟无响应状态30秒
            submitButton.Enabled = false; // 禁用按钮防止重复点击
            ageTextBox.Enabled = false;   // 禁用文本框

            // 显示提示信息
            var statusLabel = new Label();
            statusLabel.Text = "正在处理中...";
            statusLabel.Location = new Point(120, 105);
            statusLabel.Size = new Size(150, 25);
            statusLabel.ForeColor = Color.Blue;
            this.Controls.Add(statusLabel);
            this.Refresh();

            // 使用新线程执行延时操作，避免阻塞UI线程
            Thread.Sleep(30000); // 延迟30秒

            // 更新UI - 移除状态标签并显示最终消息
            this.Invoke(new Action(() =>
            {
                this.Controls.Remove(statusLabel);
                label.Text = "我不会，长大后再学";
                label.Location = new Point(20, 60);
                label.Size = new Size(350, 50);
                
                // 移除其他控件
                this.Controls.Remove(ageTextBox);
                this.Controls.Remove(submitButton);
                
                // 调整窗体大小以适应新内容
                this.Size = new Size(400, 180);
            }));
        }

        [STAThread]
        static void Main()
        {
            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new AgeCalculatorForm());
        }
        
    }
}
