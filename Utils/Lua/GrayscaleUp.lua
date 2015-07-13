require 'CLRPackage'
import("System.Windows.Forms")
import("System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")
import("System.Drawing")
import("System")
import("System.IO")
import("LuaCS")

incrementValue = 5;
maximumBrightness = 255;


function drawPixels (frame)
	frame = frame % (maximumBrightness / incrementValue)
	returnTable = {}
	r = {}
	g = {}
	b = {}
	for i = 1, pixelCount do
		r[i] = math.min(maximumBrightness,   ((i+frame)*incrementValue));
		g[i] = math.min(maximumBrightness,   ((i+frame)*incrementValue));
		b[i] = math.min(maximumBrightness,   ((i+frame)*incrementValue));
	end
	returnTable[0] = r;
	returnTable[1] = g;
	returnTable[2] = b;
	return returnTable
end


configForm=nil
buttonOk = nil
buttonCancel = nil
numIncrementValue=nil
numMaxBrightness=nil

lblIncrement=nil
lblMaxBright=nil

function handleButton(sender, data)
	if sender.Text == "Accept" then
		incrementValue = numIncrementValue.Value
		maximumBrightness = numMaxBrightness.Value
	else
	
	end
	configForm:Hide()
end

function handleScroll(sender, data)
	if sender.Name == "numIncrementValue" then
		lblIncrement.Text = "Increment Value: " .. sender.Value
	end
	if sender.Name == "numMaxBrightness" then
		lblMaxBright.Text = "Maximum Brightness: " .. sender.Value
	end
	configForm:Refresh()
end

function displayConfig()
	if configForm == nil then
		configForm=Form()
		
		buttonOk = Button()
		buttonOk.Text = "Accept"
		buttonOk.Location = Point(12, 190)
		buttonOk.MouseUp:Add(handleButton)
		
		buttonCancel = Button()
		buttonCancel.Text = "Cancel"
		buttonCancel.Location = Point(93, 190)
		buttonCancel.MouseUp:Add(handleButton)
		
		
		
		numIncrementValue = HScrollBar()
		numIncrementValue.Name = "numIncrementValue"
		numIncrementValue.Size = Size(160,20)
		numIncrementValue.Location = Point(15,43)
		numIncrementValue.Value = 4
		numIncrementValue.Minimum = 0
		numIncrementValue.Maximum = 50
		numIncrementValue.Scroll:Add(handleScroll)
		
		lblIncrement = Label()
		lblIncrement.Location = Point(12,30)
		lblIncrement.Size = Size(160,20)
		lblIncrement.BackColor = Color.Transparent
		lblIncrement.Text = "Increment Value: "
		
		
		numMaxBrightness = HScrollBar()
		numMaxBrightness.Name = "numMaxBrightness"
		numMaxBrightness.Size = Size(160,20)
		numMaxBrightness.Location = Point(15,89)
		numMaxBrightness.Value = 4
		numMaxBrightness.Minimum = 0
		numMaxBrightness.Maximum = 255
		numMaxBrightness.Scroll:Add(handleScroll)
		
		lblMaxBright = Label()
		lblMaxBright.Location = Point(12,76)
		lblMaxBright.Size = Size(240,20)
		lblMaxBright.BackColor = Color.Transparent
		lblMaxBright.Text = "Maximum Brightness: "
		
		
		configForm.Size=Size(200, 300)
		configForm.Text="Configuration V0.0.1"
		configForm.HelpButton = false
		configForm.MaximizeBox = false
		configForm.MinimizeBox = false
		configForm.AcceptButton = buttonClose
		
		configForm.Controls:Add(numIncrementValue)
		configForm.Controls:Add(lblIncrement)
		configForm.Controls:Add(numMaxBrightness)
		configForm.Controls:Add(lblMaxBright)
		configForm.Controls:Add(buttonOk)
		configForm.Controls:Add(buttonCancel)
	end
	configForm:ShowDialog()
end
