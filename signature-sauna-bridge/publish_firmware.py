import os
import os.path
from os import path
import math
import re


try:
    import boto3
    import botocore 
except ImportError:
  print ("Trying to Install required module: boto3\n")
  os.system('python -m pip install boto3')
# -- above lines try to install requests module if not present
# -- if all went well, import required module again ( for global access)
import boto3
import botocore

bucket_name = 'legacy-tablet-android-images'
def convert_size(size_bytes):
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return "%s %s" % (s, size_name[i])

def firmware_exists_on_s3(current_version):
    s3 = boto3.resource('s3')

    firmware_exists_on_s3 = False

    try:
        s3.Object(bucket_name, f'signature-bridge/{current_version}.bin').load()
        firmware_exists_on_s3 = True

    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == "404":
            # The object does not exist.
            firmware_exists_on_s3 = False
        else:
            # Something else has gone wrong.
            raise
    return firmware_exists_on_s3


# .pio\build\esp32dev\firmware.bin
# binary_path = """.pio/build/esp32dev/firmware.bin"""
binary_path = os.path.join('.pio', 'build', 'esp32dev', 'firmware.bin')
if(path.exists(binary_path)):
    # remove onld fimware file
    print("removing old firmware file")
    os.remove(binary_path)

# build the binary
os.system('pio run')

# os.system(os.path.join('docker_build.sh'))
print(binary_path)
# os.chdir("integration_tests")
# test_result = os.system('npm run test')
# print(test_result)
# if test_result > 0 :
#     print("Integration tests failed")
#     exit()
# else:
#     os.chdir("..")

print("Binary exists:"+str(path.exists(binary_path)))
print("Binary is file :"+str(path.isfile(binary_path)))
if  path.exists(binary_path):
    print("Binary size :"+str(convert_size(os.stat(binary_path).st_size)))


    id_header = open('src/main.ino', 'r')
    current_version = ""
    for line in id_header.readlines():
        if re.search('\sFIRMWARE_VERSION\s', line, re.I):
            ver_str = line.split('=')[1]
            ver_str = ver_str.replace('"', '').strip(';').strip()
            print(f"Firmware version: {ver_str.strip(';')}")
            current_version = ver_str.strip(';')



    if firmware_exists_on_s3(current_version):
        print(f"Firmware with same version exists on s3. Cannot upload")
    else:
        print(f"Firmware does not exists on s3. Uploading...")
        s3 = boto3.resource('s3')
        s3.meta.client.upload_file(Filename=binary_path, Bucket=bucket_name, Key=f'signature-bridge/{current_version}.bin')
        if(firmware_exists_on_s3(current_version)):
            print("Done uploading. Success!")
            print("Please update bridge_firmware_mappingv1.json for mapping")
        else:
            print("could not upload")
else:
    print("binary does not exist. most probably build did not happen")
