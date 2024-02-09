# Take the contents of the Payload folder and convert it into a C Header file

from edk2toollib.utility_functions import export_c_type_array

with open("./RecoveryPayload.h", "w", newline='\r\n') as output_fs:

    # By setting newline to '\r\n' we ensure that the output file is in Windows format when write converts the '\n' to '\r\n'
    output_fs.write("#ifndef _RECOVERY_PAYLOAD_H_\n")
    output_fs.write("#define _RECOVERY_PAYLOAD_H_\n\n")
    output_fs.write("#include <Uefi.h>\n\n")


    with open("Payload/dbUpdate.bin", "rb") as payload_fs:
        export_c_type_array(payload_fs, "mDbUpdate", output_fs, indent='  ')

    output_fs.write("\n")

    with open("Payload/windows uefi ca 2023.cer", "rb") as payload_fs:
        export_c_type_array(payload_fs, "mTargetCertificate", output_fs, indent='  ')

    output_fs.write("#endif // _RECOVERY_PAYLOAD_H_\n")
