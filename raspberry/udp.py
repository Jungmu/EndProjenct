import socket

UDP_IP = '' 
UDP_PORT = 3333 
 
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 

sock.bind((UDP_IP, UDP_PORT))
 
while True:
	data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
	if data == "l":
		print "left !!!"
	if data == "r":
		print "right !!?"
	if data == "u":
		print "up~~~"
	if data == "d":
		print "down...."
