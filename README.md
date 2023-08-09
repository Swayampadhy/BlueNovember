# BlueNovember

### Credits to ZeroPointSecurity and IRED.team for helping me learn about offensive kernel drivers.

### About
An offensive driver created with the intent to evade kernel level security solutions such as anti-viruses and built-in protective measures such as kernel callbacks and process protection.It can also be used to change process token privileges and enforce Driver signatures.This is a client-driver model. The client issues commands and the driver carries them out all the while returning the results.

### Objectives Of Driver

1. Create Persistence of an attacker in the system
2. Change kernel protection of processes - i.e Add or remove protection
3. Change privileges of processes
4. Disable kernel callbacks
5. Enforce driver signature

**NOTE-** The changes made by the driver is made in memory i.e it lasts upto system restart or shutdown. Also using the driver has a high risk to crash systems so please try to avoid using this unless absolutely necessary.

### Set-up And Working

Step-1 :
----
Type `bcdedit /set testsigning on` in an admin terminal to disable checking for a valid code-signing certificate.

Step-2 :
----
A.  Build the solution and put the contents of the debug folder to **C:\BlueNovember** in the victim machine.

**OR**

B.  Download the release , unzip and place it in **C:\BlueNovember**.


Step-3 :
-----
Type `sc create BlueNovember type= kernel binPath= C:\BlueNovember\BlueNovember.sys` to create a service of the driver.
![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/b0279f91-8885-4869-92a0-fe411e75a303)

Then type `sc qc BlueNovember` to check the process status.
![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/3c4a12cd-cb64-4a16-9df5-57afa8930614)


Step-4 :
------

Type `sc start BlueNovember` to start the service.
![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/6d9a8cff-c1fb-494e-abfc-e47763b279c4)

Step-5 :
------
Go to **C:\BlueNovember** in terminal and type - `.\Client.exe -<option> <Optional: pid> ` to run the driver.

STEP-6 (Optional) :
-----
If you want to delete the driver , type - `sc stop BlueNovember` to stop the driver service and then type - `sc delete BlueNovember` to delete the service. After that you can remove the files in **C:\BlueNovember** .

### Execution Options 

1. `-pp <PID of processes>` : **Protect processes** - This option will ensure kernel level protection for the processes with the given PIDs.

2. `-up <PID of processes>` : **Unprotect Process** - This option will remove kernel level protection for the processes with the given PIDs.

3. `-t <PID of process>`    : **Grant all privilege** - This option will grant all privileges to the process with the given PID.

4. `-l <PID of process>`    : **Enumerate kernel callbacks** - This option will enumerate all kernel callbacks issued by the process.

5. `-r <PID of process>`    : **Remove callbacks** - This option removes all kernel callbacks ot the given process.

6. `-ci `                   : **Enumerate DSE** - This option enumerates Driver signature enforcement(DSE) of the driver.

7. `-ciE`                   : **Enable DSE** - This option enables DSE of the driver.

8. `-ciD`                   : Disable DSE - This option disables DSE of the driver.

### To-do -

1. Solve windows 22H2 randomly crashing after issuing commands.
2. Add support for more windows versions.
3. Streamlining the code more.
4. Adding more functionalities.
