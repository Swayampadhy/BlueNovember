# BlueNovember

### Credits to ZeroPointSecurity and IRED.team for helping me learn about offensive kernel drivers.

### About
-----
-----
An offensive driver created with the intent to evade kernel level security solutions such as anti-viruses and built-in protective measures such as kernel callbacks and process protection.It can also be used to change process token privileges and enforce Driver signatures.This is a client-driver model. The client issues commands and the driver carries them out all the while returning the results.

### Theory
-----
-----

### Objectives Of Driver
------
-----
1. Create Persistence of an attacker in the system
2. Change kernel protection of processes - i.e Add or remove protection
3. Change privileges of processes
4. Disable kernel callbacks
5. Enforce driver signature

**NOTE-** The changes made by the driver is made in memory i.e it lasts upto system restart or shutdown. Also using the driver has a high risk to crash systems so please try to avoid using this unless absolutely necessary.You can use **Process Explorer** from **sysinternals** to get PIDS.

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
-------
-------
1. `-pp <PID of processes>` : **Protect processes** - This option will ensure kernel level protection for the processes with the given PIDs.

  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/4f4beab3-47e0-4194-b50f-b6e45eed00dc)

  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/cb077b79-ccda-480b-936f-a8bf83e8e387)

2. `-up <PID of processes>` : **Unprotect Process** - This option will remove kernel level protection for the processes with the given PIDs.

  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/41b97ce6-c4ac-4193-99ee-40e80bc7eea8)


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/38ec40fc-e442-4e99-9427-455dfe0bb17d)

3. `-t <PID of process>`    : **Grant all privilege** - This option will grant all privileges to the process with the given PID.


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/64e2f00a-263f-4aad-badc-1cb6c53cb259)


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/caef801b-b2d0-4417-a441-7078b5ba62fb)

4. `-l `    : **Enumerate kernel callbacks** - This option will enumerate all kernel callbacks issued by the processes.

  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/5f1948e5-8b72-485a-bc46-55ad5ebfa7b6)

5. `-r <process no>`    : **Remove callbacks** - This option removes all kernel callbacks issued by the process given.

  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/b4b56d98-ea57-448d-abfa-914aa1cc3133)


6. `-ci `                   : **Enumerate DSE** - This option enumerates Driver signature enforcement(DSE) of the driver.


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/1812d277-8990-47e8-86b5-0ee61b4575ed)


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/291d60f7-e34d-4bae-8eed-e42d4f96127e)

7. `-ciE`                   : **Enable DSE** - This option enables DSE of the driver.

![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/de029a60-9ce7-462c-b949-c195682a177a)


8. `-ciD`                   : **Disable DSE** - This option disables DSE of the driver.


### To-do -
-------
1. Solve windows 22H2 randomly crashing after issuing commands.
2. Streamlining the code more.
3. Adding more functionalities.
