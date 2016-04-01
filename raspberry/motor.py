import socket
import RPi.GPIO as GPIO
import time
pin = 18
pin1 = 13

GPIO.setmode(GPIO.BCM)
GPIO.setup(pin, GPIO.OUT)

GPIO.setup(pin1,GPIO.OUT)

p = GPIO.PWM(pin,50)
p1 = GPIO.PWM(pin1,50)

var =1

degree_x = 7.5
degree_y = 7.5

UDP_IP=''
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

sock.bind((UDP_IP,UDP_PORT))

p.start(degree_x)
p1.start(degree_y)

#time.sleep(1)

#p.stop()
#p1.stop()



while True:
        data,addr = sock.recvfrom(1024)
        print("Receive:"),data
        var = data


        if var == "r":
                if degree_x < 10:
                        degree_x = degree_x + 0.1
                        GPIO.setmode(GPIO.BCM)
                        GPIO.setup(pin, GPIO.OUT)
                        p.start(degree_x)
        elif var == "l":
                if degree_x > 5:
                        degree_x = degree_x - 0.1
                        GPIO.setmode(GPIO.BCM)
                        GPIO.setup(pin, GPIO.OUT)
                        p.start(degree_x)
        elif var == "u":
                if degree_y < 10:
                        degree_y = degree_y + 0.1
                        GPIO.setmode(GPIO.BCM)
                        GPIO.setup(pin1, GPIO.OUT)
                        p1.start(degree_y)
        elif var == "d" :
                if degree_y > 5 :
                        degree_y = degree_y - 0.1
                        GPIO.setmode(GPIO.BCM)
                        GPIO.setup(pin1, GPIO.OUT)
                        p1.start(degree_y)
        elif var == "s":
                time.sleep(1)
                GPIO.cleanup()
                print "hahaha"

