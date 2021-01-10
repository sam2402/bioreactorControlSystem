from serial import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from tkinter import Label, Scale, Button, HORIZONTAL, Tk, ttk, DoubleVar, Frame, TOP, BOTH
import tkinter as tk
import matplotlib.pyplot as pltlib
import matplotlib
matplotlib.use("TkAgg")

arduino = Serial(port='/tmp/simavr-uart0', baudrate=9600, timeout=.1)


class ValueList:
    def __init__(self):
        self.list = []

    def append(self, val):
        if len(self.list) > 400:
            self.list.pop(0)
        self.list.append(val)

    def appendIncrement(self, x):
        if self.length() == 0:
            self.append(0)
        else:
            last = self.getList()[-1]
            self.append(last+x)

    def length(self):
        return len(self.list)

    def getList(self):
        return self.list

    def head(self):
        try:
            return self.getList()[-1]
        except:
            return None


class BioreactorApp():
    def __init__(self, *args, **kwargs):
        self.root = Tk()
        self.root.geometry("3000x3000")
        self.root.wm_title("Bioreactor")
        self.pastValues = {"Temperature (°C)": ValueList(
        ), "Stirrer RPM": ValueList(), "pH": ValueList()}
        self.times = {"Temperature (°C)": ValueList(
        ), "Stirrer RPM": ValueList(), "pH": ValueList()}

        container = Frame(self.root)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        self.frames = {}
        for F in (SetPointsPage, GraphsPage):
            page_name = F.__name__
            frame = F(parent=container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")
        self.show_frame("SetPointsPage")

    def show_frame(self, page_name):

        frame = self.frames[page_name]
        frame.tkraise()

    def read(self):
        data = arduino.readline().strip().decode('ascii')
        if(data != ''):
            arduinoData = data.split(" ")
            values = {
                "Temperature (°C)": arduinoData[0], "Stirrer RPM": arduinoData[1], "pH": arduinoData[2]}
        else:
            values = {"Temperature (°C)": self.pastValues["Temperature (°C)"].head(),
                      "Stirrer RPM": self.pastValues["Stirrer RPM"].head(),
                      "pH": self.pastValues["pH"].head()}
        return values


class SetPointsPage(Frame):
    def __init__(self, parent, controller):
        Frame.__init__(self, parent)
        self.controller = controller
        self.oldHeating = None
        self.oldStirring = None
        self.oldPh = None

        title = Label(
            self,
            text="Bioreactor Control System",
            font="Helvetica 25 bold"
        )
        title.pack(pady=10)
        self.createSliderUI("pH", 3, 7, 0.1)
        self.createSliderUI("Stirrer RPM", 500, 1500, 20)
        self.createSliderUI("Temperature (°C)", 25, 35, 0.2)

        toggleFrameButton = Button(
            self,
            text="View Graphs",
            command=lambda: controller.show_frame("GraphsPage"),
        )
        toggleFrameButton.pack(pady=10)

    def write(self, subsystemName, value):
        values = {"Temperature (°C)": self.oldHeating,
                  "Stirrer RPM": self.oldStirring, "pH": self.oldPh}
        values[subsystemName] = value
        arduino.write(bytes(str(values["Temperature (°C)"])+" " +
                            str(values["Stirrer RPM"])+" "+str(values["pH"]), 'utf-8'))
        if subsystemName == "Temperature (°C)":
            self.oldHeating = value
        elif subsystemName == "Stirrer RPM":
            self.oldStirring = value
        elif subsystemName == "pH":
            self.oldPh = value

    def updateVal(self, label, parameter):
        values = self.controller.read()
        label["text"] = "Current " + parameter + ": " + str(values[parameter])
        self.controller.pastValues[parameter].append(values[parameter])
        self.controller.times[parameter].appendIncrement(0.2)
        self.controller.root.after(200, self.updateVal, label, parameter)

    def createSliderUI(self, subsystemName, minVal, maxVal, resolution):
        scaleLabel = Label(
            self,
            text="Set %s - %d to %d" % (subsystemName, minVal, maxVal),
            font="Helvetica 13 bold",
            anchor="w",
            padx=7,
        )
        scale = Scale(
            self,
            from_=minVal,
            to=maxVal,
            orient=HORIZONTAL,
            resolution=resolution,
            command=lambda val, name=subsystemName: self.write(name, val),
            length=300
        )

        scale.set(sum([minVal, maxVal])/2)
        scaleLabel.pack(fill="both")
        scale.pack()

        currentPhLabel = Label(self)
        currentPhLabel.pack()
        self.updateVal(currentPhLabel, subsystemName)


class GraphsPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        self.graph = None
        self.fig = None
        
        self.selectedGraph = "pH"
        title = Label(
            self,
            text="Subsystem Graphs",
            font="Helvetica 25 bold"
        )
        title.pack(pady=10)
        self.plot()
	
        pHGraphButton = Button(
            self,
            text="pH Graph",
            command=lambda: self.setSelectedGraph("pH")
	    )
        pHGraphButton.pack()
        StirrerGraphButton = Button(
            self,
            text="Stirrer Graph",
            command=lambda: self.setSelectedGraph("Stirrer RPM")
	    )
        StirrerGraphButton.pack()
        temperatureGraphButton = Button(
            self,
            text="Temperature Graph",
            command=lambda: self.setSelectedGraph("Temperature (°C)")
	    )
        temperatureGraphButton.pack()

        toggleFrameButton = Button(
            self,
            text="View Control Pane",
            command=lambda: controller.show_frame("SetPointsPage"),
        )
        toggleFrameButton.pack(pady=(0, 10))

    def setSelectedGraph(self, newGraph):
        self.selectedGraph=newGraph
        self.plot()

    def updateGraph(self):
        times = self.controller.times[self.selectedGraph].getList()
        values = self.controller.pastValues[self.selectedGraph].getList()
        self.line1.set_data(times, values)
        ax = self.canvas.figure.axes[0]
        ax.relim()
        ax.autoscale_view(True,True,True)       
        self.canvas.draw()
        self.controller.root.after(500, self.updateGraph)

    def plot(self):
        self.canvasFig=pltlib.figure(1)
        Fig = matplotlib.figure.Figure(figsize=(15,5),dpi=100)
        FigSubPlot = Fig.add_subplot(111)
        FigSubPlot.set_ylabel(self.selectedGraph)
        FigSubPlot.set_xlabel("time (s)")
        times = self.controller.times[self.selectedGraph].getList()
        values = self.controller.pastValues[self.selectedGraph].getList()
        self.line1, = FigSubPlot.plot(times,values,'r-')
        self.canvas = matplotlib.backends.backend_tkagg.FigureCanvasTkAgg(Fig, master=self)
        self.canvas.draw()
        pl=self.canvas.get_tk_widget()
        pl.place(x=0, y=210)
        self.updateGraph()

if __name__ == "__main__":
    app = BioreactorApp()
    app.root.mainloop()
