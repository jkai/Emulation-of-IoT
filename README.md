==============
 Introduction
==============

This is a program emulate an IoT system, due to time limitation the device functionalities are defined roughly.
- Controller:

	Parent process communicates the Cloud process using FIFO and communicates Child process through message queue and signal to fetch device infomation and to manipulate devices based on user input in Cloud process.

	Child process communicates the devices and parent process through message queue, communication with parent process is triggered by signals.

- Cloud:
	
	Parent process takes user input and communicates child process.

	Child process communicates the controller through FIFO and outputs the required information to screen

- Sensor:
	
	Returns a random number from 0 to 60 as measurement, name and thresold are defined base user input during initialization.

	In this assignment, Sensors are treated as tempurature sensor which triggers AC.

- Actuator:

	In this assignment, actuator are treated as AC. An available AC will be turn on if a threshold crossing ocurrs. If all the ACs are on, no action will be taken.

	It will be shut off by using PUT command in Cloud process.

=======
 Usage
=======
A makefile is provided.

Recommand start order, for the better verification experience (Prevent FIFO blocking issue).

[IMPORTANT!!!]
Don't shut down cloud processes before shut down controller, because it will terminate controller parent process to prevent Ctrl-C feature. Then you will have to kill it through pid mannully.

1. Controller:
	e.g.	./Controller controller1
	Args: [controller name]

	Comments: After parent and child processes started, Ctrl-C will terminate controller and all running devices. 

2. Cloud:
	e.g.	./Cloud

	Comments: To verify the GET/PUT feature easier, run only a sensor with 99 threshold to prevent the confusion during user input. (Just for convinience)

	By input q, the processes will be shut down.

3. Actuator:

	e.g.	./Actuator AC1
	Args: [Actuator name]

	Comment: Will quit by using PUT command in Cloud

4. Sensor:

	e.g.	./Sensor sensor1 50
	Args:	[Sensor name, threshold]

	Comment: set threshold>60 will prevent threshold crossing, which will be helpful for verifying GET/PUT features (easier user input).
