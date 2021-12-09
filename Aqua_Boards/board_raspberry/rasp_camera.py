import time
import pygame
import os
import paho.mqtt.client as mqtt
import requests


# set displayed preview image size (must be less than screen size to allow for menu!!)
# recommended 640x480, 800x600, 1280x960
preview_width  = 800
preview_height = 600

# set default values (see limits below)
mode        = 1       # set camera mode ['manual','normal','sport']
speed       = 13      # position in shutters list
gain        = 1       # set gain
brightness  = 0       # set camera brightness
contrast    = 70      # set camera contrast
ev          = 0       # eV correction
blue        = 12      # blue balance
red         = 15      # red balance
extn        = 0       # still file type
vlen        = 10      # video length in seconds
fps         = 25      # video fps
vformat     = 4       # set video format
codec       = 0       # set video codec
tinterval   = 5       # time between timelapse shots in seconds
tshots      = 5       # number of timelapse shots
frame       = 0       # set to 1 for no frame (i.e. if using Pi 7" touchscreen)
saturation  = 10      # picture colour saturation
meter       = 0       # metering mode
awb         = 1       # auto white balance mode, off, auto etc
sharpness   = 1       # set sharpness level
denoise     = 0       # set denoise level

# inital parameters
zx          = int(preview_width/2)
zy          = int(preview_height/2)
zoom        = 0

# default directories and files
config_file = "/home/pi/PiLCConfig3.txt"

# Camera max exposure (Note v1 is currently 1 second not the raspistill 6 seconds)
# whatever value set it MUST be in shutters list !!
max_v1      = 1
max_v2      = 10
max_hq      = 239

# data
vwidths      = [640,800,1280,1280,1920,2592,3280,4056]
vheights     = [480,600, 720, 960,1080,1944,2464,3040]
v_max_fps    = [90 , 40,  40,  40,  30,  20,  20,  20]
shutters     = [-2000,-1600,-1250,-1000,-800,-640,-500,-400,-320,-288,-250,-240,-200,-160,-144,-125,-120,-100,-96,-80,-60,-50,-48,-40,-30,-25,-20,-15,-13,-10,-8,-6,-5,-4,-3,
                0.4,0.5,0.6,0.8,1,1.1,1.2,2,3,4,5,6,7,8,9,10,15,20,25,30,40,50,60,75,100,120,150,200,220,230,239]

# check config_file exists, if not then write default values
if not os.path.exists(config_file):
    points = [mode,speed,gain,brightness,contrast,frame,red,blue,ev,vlen,fps,vformat,codec,tinterval,tshots,extn,zx,zy,zoom,saturation,meter,awb,sharpness,denoise]
    with open(config_file, 'w') as f:
        for item in points:
            f.write("%s\n" % item)

# read config_file
config = []
with open(config_file, "r") as file:
   line = file.readline()
   while line:
      config.append(line.strip())
      line = file.readline()
config = list(map(int,config))


mode        = config[0]
speed       = config[1]
gain        = config[2]
brightness  = config[3]
contrast    = config[4]
fullscreen  = config[5]
red         = config[6]
blue        = config[7]
ev          = config[8]
vlen        = config[9]
fps         = config[10]
vformat     = config[11]
codec       = config[12]
tinterval   = config[13]
tshots      = config[14]
extn        = config[15]
zx          = config[16]
zy          = config[17]
zoom        = config[18]
saturation  = config[19]
meter       = config[20]
awb         = config[21]
sharpness   = config[22]
denoise     = config[23]

# Check for Pi Camera version
if os.path.exists('test.jpg'):
    os.rename('test.jpg', 'oldtest.jpg')
rpistr = "libcamera-jpeg -n -t 1000 -e jpg -o test.jpg"
os.system(rpistr)
rpistr = ""
time.sleep(2)
if os.path.exists('test.jpg'):
    imagefile = 'test.jpg'
    image = pygame.image.load(imagefile)
    igw = image.get_width()
    igh = image.get_height()
    if igw == 2592:
        Pi_Cam = 1
        max_shutter = max_v1
    elif igw == 3280:
        Pi_Cam = 2
        max_shutter = max_v2
    elif igw > 3280:
        Pi_Cam = 3
        max_shutter = max_hq
else:
    Pi_Cam = 0
    max_shutter = max_v1
    
# MQTT client 

topic = "deviceid/Board_A/cmd/#"
send_topic = "deviceid/Board_A/evt/#"
server = "54.90.184.120"
photo_cycle = 0
photo_time = []
pres_sec = 0
past_sec = 0
cycle = 0

def take_photo():
    global photo_cycle, photo_time
    photostr = "libcamera-jpeg -n -t 100 -e jpg -o photo{}.jpg".format(str(photo_cycle))
    os.system(photostr)
    photostr = ""
    time.sleep(2)
            
    photo_time.append(time.strftime('%y-%m-%d %H:%M:%S'))
    print("Take PHOTO Number%d - %s" %(photo_cycle, photo_time[photo_cycle]))
    
    # image to web
    webstr = "photo{}.jpg".format(str(photo_cycle))
    files = open(webstr, 'rb')
    upload = {'file':files}
    res = requests.post('http://54.84.225.1:5000/input', files = upload)
    print(res)
    webstr = "photo{}".format(str(photo_cycle))
    photoweb = 'http://54.84.225.1:5000/{}'.format(webstr)
    client.publish(send_topic.replace("#", "photo"), photoweb)
    print(photoweb)
            
    photo_cycle = photo_cycle + 1
    if photo_cycle > 9:
        photo_cycle = 0
        photo_time.clear()

def on_connect(client, userdata, flags, rc):
    print("Connected with RC : " + str(rc))
    client.subscribe(topic)


def on_message(client, userdata, msg):
    global cycle, past_sec
    if msg.topic == topic.replace("#", "bab"):
        bab = int(msg.payload)
        if bab == 1:
            take_photo()
            print("Use bab")
    if msg.topic == topic.replace("#", "cycle"):
        cycle = int(msg.payload)
        print("Use cycle : %d" %cycle)
        if cycle > 0:
            past_sec = int(time.time())
            print("Past sec : %d" %past_sec)

client = mqtt.Client()
client.connect(server, 1883, 60)
client.on_connect = on_connect
client.on_message = on_message


forever = 0
while forever == 0:
    client.loop()

    pres_sec = int(time.time())

    if (pres_sec - past_sec) > (cycle*10) and cycle != 0:
        #client.publish(topic.replace("#", "bab"), str(1))
        take_photo()
        past_sec = pres_sec
    

    

