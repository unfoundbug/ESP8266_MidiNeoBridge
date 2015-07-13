require 'CLRPackage'
import("System.Windows.Forms")
import("System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")
import("System.Drawing")
import("System")
import("System.IO")
import("LuaCS")

TailDecayRateValue = 16;
maximumBrightness = 255;
EyeMoveSpeed=3
thisTailDecayRate = TailDecayRateValue;
thisBrightness = maximumBrightness
thisEyeMoveSpeed=EyeMoveSpeed;
eyePoint = 1;
eyeDirection = 1;
	r = {}
	g = {}
	b = {}
	
function drawPixels (frame)
localBright = thisBrightness
localTailDecayRate = thisTailDecayRate
localEyeMoveSpeed=thisEyeMoveSpeed
	
	if frame % thisEyeMoveSpeed == 0then
		eyePoint = eyePoint + eyeDirection;
		if eyePoint < 1 then
			eyePoint = 1 
			eyeDirection = 1
		end
		if eyePoint > pixelCount then
			eyePoint = pixelCount 
			eyeDirection = -1
		end
	end
	
	returnTable = {}
	for i = 1, pixelCount do
		if r[i] == nil then
			r[i] = 0
			g[i] = 0
			b[i] = 0
		end
		r[i] = math.max(0, r[i] - localTailDecayRate)
	end
	r[eyePoint] = localBright;
	returnTable[1] = r;
	returnTable[2] = g;
	returnTable[3] = b;
	return returnTable
end

for i = 1, pixelCount do
		r[i] = math.max(0, r[i] - localTailDecayRate)
	end



configForm=nil
buttonOk = nil
buttonCancel = nil
numTailDecayRateValue=nil
numMaxBrightness=nil
numEyeMoveSpeed=nil
lblTailDecayRate=nil
lblMaxBright=nil
lblEyeMoveSpeed=nil


function handleButton(sender, data)
	if sender.Text == "Accept" then
		TailDecayRateValue = numTailDecayRateValue.Value
		maximumBrightness = numMaxBrightness.Value
		EyeMoveSpeed = numEyeMoveSpeed.Value
	else
		numTailDecayRateValue.Value = TailDecayRateValue
		numMaxBrightness.Value = maximumBrightness
		numEyeMoveSpeed.Value = EyeMoveSpeed
		thisTailDecayRate = TailDecayRateValue
		thisBrightness = maximumBrightness
		thisEyeMoveSpeed = EyeMoveSpeed;
	end
	configForm:Close()
end

function handleScroll(sender, data)
	if sender.Name == "numTailDecayRateValue" then
		lblTailDecayRate.Text = "TailDecayRate Value: " .. sender.Value
		thisTailDecayRate = sender.Value
	end
	if sender.Name == "numMaxBrightness" then
		lblMaxBright.Text = "Maximum Brightness: " .. sender.Value
		thisBrightness=sender.Value
	end
	if sender.Name == "numEyeMoveSpeed" then
		lblEyeMoveSpeed.Text = "EyeMoveSpeed: " .. sender.Value
		thisEyeMoveSpeed=sender.Value
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
		
		
		
		numTailDecayRateValue = HScrollBar()
		numTailDecayRateValue.Name = "numTailDecayRateValue"
		numTailDecayRateValue.Size = Size(160,20)
		numTailDecayRateValue.Location = Point(15,43)
		numTailDecayRateValue.Minimum = 0
		numTailDecayRateValue.Maximum = 50
		numTailDecayRateValue.Value = TailDecayRateValue
		numTailDecayRateValue.Scroll:Add(handleScroll)
		
		lblTailDecayRate = Label()
		lblTailDecayRate.Location = Point(12,30)
		lblTailDecayRate.Size = Size(160,20)
		lblTailDecayRate.BackColor = Color.Transparent
		lblTailDecayRate.Text = "TailDecayRate Value: " .. TailDecayRateValue
		
		
		numMaxBrightness = HScrollBar()
		numMaxBrightness.Name = "numMaxBrightness"
		numMaxBrightness.Size = Size(160,20)
		numMaxBrightness.Location = Point(15,89)
		numMaxBrightness.Minimum = 0
		numMaxBrightness.Maximum = 270
		numMaxBrightness.Value = maximumBrightness
		numMaxBrightness.Scroll:Add(handleScroll)
		
		lblMaxBright = Label()
		lblMaxBright.Location = Point(12,76)
		lblMaxBright.Size = Size(240,20)
		lblMaxBright.BackColor = Color.Transparent
		lblMaxBright.Text = "Maximum Brightness: " .. maximumBrightness
		
		numEyeMoveSpeed = HScrollBar()
		numEyeMoveSpeed.Name = "numEyeMoveSpeed"
		numEyeMoveSpeed.Size = Size(160,20)
		numEyeMoveSpeed.Location = Point(15,134)
		numEyeMoveSpeed.Minimum = 0
		numEyeMoveSpeed.Maximum = 270
		numEyeMoveSpeed.Value = EyeMoveSpeed
		numEyeMoveSpeed.Scroll:Add(handleScroll)
		
		lblEyeMoveSpeed = Label()
		lblEyeMoveSpeed.Location = Point(12,121)
		lblEyeMoveSpeed.Size = Size(240,20)
		lblEyeMoveSpeed.BackColor = Color.Transparent
		lblEyeMoveSpeed.Text = "EyeMoveSpeed: " .. EyeMoveSpeed
		
		configForm.Size=Size(200, 300)
		configForm.Text="Configuration V0.0.2"
		configForm.HelpButton = false
		configForm.MaximizeBox = false
		configForm.MinimizeBox = false
		configForm.AcceptButton = buttonClose
		
		configForm.Controls:Add(numTailDecayRateValue)
		configForm.Controls:Add(lblTailDecayRate)
		configForm.Controls:Add(numMaxBrightness)
		configForm.Controls:Add(lblMaxBright)
		configForm.Controls:Add(numEyeMoveSpeed)
		configForm.Controls:Add(lblEyeMoveSpeed)
		configForm.Controls:Add(buttonOk)
		configForm.Controls:Add(buttonCancel)
	end
	configForm:ShowDialog()
end
