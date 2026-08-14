# Marisa Patel, 400393635, patem156

import serial # Importing serial and math
import math

s = serial.Serial('COM3',115200) # Using the Port and Baud Rate

s.open
s.reset_output_buffer()
s.reset_input_buffer()

f = open("2dx3points.xyz", "w") 
step = 0
x = 0 # This is the initial x-displacement(mm)
increment = 250 # This is the x-displacement steps(mm)
num_inc = int(input("Enter Number of Scans:"))
count=0
while(count<num_inc): # While loop
    raw = s.readline()
    data = raw.decode("utf-8") # Here we are decoding the byte input from UART into a string 
    data = data[0:-2] # Here we are removing the carriage return and new line from the string
    if (data.isdigit() == True): # If statement
        angle = (step/512)*2*math.pi # Here we will get the angle depending on the motor rotation
        r = int(data)
        y = r*math.cos(angle) # Calculate y
        z = r*math.sin(angle) # Calcualte z
        print(y) # Printing the y
        print(z) # Printing the z
        f.write('{} {} {}\n'.format(x,y,z)) # Here we are writing the obtained data in the .xyz file
        step = step+32 # The step
    if (step == 512): # Here we are resetting the number of steps after a full 360 rotation has been done
        step = 0
        x = x + increment # Here we are incrementing x
        count=count+1 # Here we are incrementing count
    print(data)
f.close() # This is to close the file when we are done to ensure that the data gets saved
