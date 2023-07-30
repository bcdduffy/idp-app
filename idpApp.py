import PySimpleGUI as sg
import glob
import serial
import time
import json
def getJson():
    a = True
    startChar = ''
    outputString = ""
    ser.write(bytes('d', 'utf-8'))
    while a:
        #print("trying Device...1")
        #ser.write(bytes('d', 'utf-8'))
        time.sleep(0.00084)
        startChar = ser.read()
        if startChar != b'\x00' and startChar != b'' and startChar != b's':
            print(startChar)
            outputString = outputString + startChar.decode("utf-8")
        if startChar.decode("utf-8") == '}':
            a = False
            parse = json.loads(outputString)
    return parse

filePath = ""
#--------------------------------------------------------------------------------
layout = [  [sg.Text("Welcome to the Weather App!")],
            [sg.Text("Please Hit Continue")],
            [sg.Button("Continue")]]
#--------------------------------------------------------------------------------
file_list_column = [
    [
        sg.Text("Bluetooth Devices"),
        #sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
        #sg.FolderBrowse(),
        sg.Button("Refresh")
    ],
    [
        sg.Listbox(
        values=[], enable_events=True, size=(40, 20), key="-FILE LIST-"
        )
    ],
    [
        sg.Button("Exit App"), sg.Button("Continue")
    ]
]
layout1 = [
    [
        sg.Column(file_list_column)
    ]
]
layout2 =   [     [sg.Text("Connecting to Bluetooth")],
                [[sg.Text("Please Wait...")]],
                [sg.Button("Connect") , sg.Button("Exit")]
            ]
layout3 = [ [sg.Button("Refresh"), sg.Text(" Current Weather", size=(30,
1), font=("Helvetica", 25)), sg.Button("Exit")],
        [sg.Text(" "),
sg.Image(r'/Users/benjamingross/Desktop/SunnyIcon.png'),sg.Text("
"), sg.Image(r'/Users/benjamingross/Desktop/WarmIcon.png')],
 [sg.Text(" Sunny ",font=("Helvetica",
15)),sg.Text(" "),sg.Text(" Mild
",font=("Helvetica", 15)) ],
 [sg.Text(" Current Temperature: ", size=(30, 1),
font=("Helvetica", 25), key = "-temp-")],
 [sg.Text(" Current Relative Humidity: ", size=(30, 1),
font=("Helvetica", 25), key = "-hum-")],
 [sg.Text(" Current Barometric pressure:", size=(50, 1),
font=("Helvetica", 25), key = "-pre-")]

 ]
#--------------------------------------------------------------------------------
# Create the window
window = sg.Window("Weather App", layout)
# Create an event loop
while True:
    event, values = window.read()
    # End program if user closes window or
    # presses the OK button
    if event == "Continue" or event == sg.WIN_CLOSED:
        break
window.close()
window = sg.Window("Weather App", layout1)
event, values = window.read()
file = glob.glob("/dev/tty.*")
fnames = file
window["-FILE LIST-"].update(fnames)
while True:
    event, values = window.read()
    # End program if user closes window or
    # presses the OK button
    if event == "Exit App" or event == sg.WIN_CLOSED:
        break
    if event == "Refresh":
        file = glob.glob("/dev/tty.*")
        fnames = file
        window["-FILE LIST-"].update(fnames)
    if event == "-FILE LIST-":
        filePathList = values["-FILE LIST-"]
        filePath = filePathList[0]
        print(filePath)
    if event == "Continue" and filePath != "":
        break
window.close()
window = sg.Window("Weather App", layout2)
ser = serial.Serial(port=filePath, baudrate=9600, timeout=.1)
ser.write(bytes('s', 'utf-8'))
time.sleep(0.000005)
startChar = ser.readline()
while True:
    event, values = window.read()
    while startChar != b's':
    print("trying Device...1")
    ser.write(bytes('s', 'utf-8'))
    print("trying Device...2")
    time.sleep(0.05)
    startChar = ser.read()
    print(startChar)
    print("trying Device...3")
    time.sleep(1)
 # End program if user closes window or
 # presses the OK button
    print("trying Device...")
    if startChar == 's':
        print("connected")
        break;

    if event == "Connect" or event == sg.WIN_CLOSED:
        break
    print("device Connected")

window.close()
window = sg.Window("Weather App", layout3, size = (600,400))
#window["-temp-"].update(anotherString)
jsonValues = ""
first = True
while True:
    event, values = window.read()
    if event == "Refresh":
        jsonValues = getJson()
        print(jsonValues)
        newTemp = "Current Temperature: " + str(jsonValues["temp"]) + "° C"
        newHum = "Current Relative Humidity: " + str(jsonValues["hm"]) + "%"
        newPre = "Current Barometric pressure: " + str(jsonValues["pre"]) + " kPa"

        window["-temp-"].update(newTemp)
        window["-hum-"].update(newHum)
        window["-pre-"].update(newPre)
 # End program if user closes window or
 # presses the OK button
    if event == "Exit" or event == sg.WIN_CLOSED:
        ser.write(bytes('e', 'utf-8'))
        ser.close()
        break
window.close()