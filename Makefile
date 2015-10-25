all: Controller Cloud Actuator Sensor

Controller: Controller.c 
	gcc Controller.c -o Controller
Cloud: Cloud.c
	gcc Cloud.c -o Cloud
Actuator: Actuator.c
	gcc Actuator.c -o Actuator
Sensor: Sensor.c
	gcc Sensor.c -o Sensor
clean:
	rm Controller, Cloud, Actuator, Sensor
