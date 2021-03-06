require 'CLRPackage'
import("System.Windows.Forms")
import("System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")
import("System.Drawing")
import("System")
import("System.IO")
import("LuaCS")

incrementValue = 6;
maximumBrightness = 128;

function HSL(h, s, l)
   if s == 0 then return l,l,l end
   h, s, l = h/256*6, s/255, l/255
   local c = (1-math.abs(2*l-1))*s
   local x = (1-math.abs(h%2-1))*c
   local m,r,g,b = (l-.5*c), 0,0,0
   if h < 1     then r,g,b = c,x,0
   elseif h < 2 then r,g,b = x,c,0
   elseif h < 3 then r,g,b = 0,c,x
   elseif h < 4 then r,g,b = 0,x,c
   elseif h < 5 then r,g,b = x,0,c
   else              r,g,b = c,0,x
   end
   return math.ceil((r+m)*255),math.ceil((g+m)*255),math.ceil((b+m)*255)
end

function drawPixels (frame)
	frame = frame % (360 / incrementValue) 
	returnTable = {}
	r = {}
	g = {}
	b = {}
	hue = (incrementValue*(frame));
	while(hue > 360) do 
		hue = hue - 360 
	end
	for i = 1, pixelCount do
		r[i], g[i], b[i] = HSL(hue, 255, maximumBrightness)
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
		--numIncrementValue.Value = incrementValue
		--numMaxBrightness.Value = maximumBrightness
	end
	configForm:Close()
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
		numIncrementValue.Minimum = 0
		numIncrementValue.Maximum = 50
		numIncrementValue.Value = incrementValue
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
		numMaxBrightness.Minimum = 0
		numMaxBrightness.Maximum = 255
		numMaxBrightness.Value = maximumBrightness
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
