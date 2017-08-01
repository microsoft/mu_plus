import os
import subprocess
import re
import sys
import tempfile
import shutil
import argparse

parser = argparse.ArgumentParser(description="This tool will find the SUT IP based off of the %SUTNAME% environment variable and update the Agentargs.txt file")
parser.add_argument("AgentArgs", help="Path to agentargs.txt file")
args = parser.parse_args()

#############################
####    Get IP of SUT    ####
#############################

ip_regex = re.compile(r"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
PIPE = -1

#Get SUTNAME
try:
    sutname = os.environ["SUTNAME"]
    print("SUT Name: " + os.environ["SUTNAME"])
except:
    raise Exception("Environment variable does not exist")

#Ping SUT
try:
    process = subprocess.Popen("ping -w 100 -n 1 -4 -a " + sutname, stdout=PIPE)
    output, unused_err = process.communicate()
    retcode = process.poll()
except:
    raise Exception("Error pinging device")

#SUT Not found
if "could not find host" in output or "Request timed out" in output:
    raise Exception("Could not find host")

ip = re.search(ip_regex,output).group()
print("IP of SUT: " + ip)

#################################
####    Update Agent Args    ####
#################################
try:
    agentfile = open(args.AgentArgs, 'r')
except:
    raise Exception("Could not find AgentArgs.txt file")

#Create backup of original Agentargs.txt before replacing it
try:
    backupfile = os.path.join(os.path.dirname(args.AgentArgs), "AgentArgsBackup.txt")
    shutil.copyfile(args.AgentArgs, backupfile)
except:
    raise Exception("Could not create backup for AgentArgs.txt")

#Replace text of file with new IP
original_text = agentfile.read()

if not bool(ip_regex.search(original_text)):
    original_text = original_text + r" -v IP_OF_SUT:000.000.000.000"
new_text = re.sub(r"IP_OF_SUT:\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}", ("IP_OF_SUT:" + ip), original_text)
agentfile.close()


#Replace original agentgile
try:
    agentfile_new = open(args.AgentArgs, 'w')
    agentfile_new.write(new_text)
    agentfile_new.close()
except IOError as (errno,strerror):
    print "I/O error({0}): {1}".format(errno, strerror)