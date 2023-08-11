# BlueNovember

### Credits to ZeroPointSecurity and IRED.team for helping me learn about offensive kernel drivers.

### About
-----
-----
An offensive driver created with the intent to evade kernel level security solutions such as anti-viruses and built-in protective measures such as kernel callbacks and process protection.It can also be used to change process token privileges and enforce Driver signatures.This is a client-driver model. The client issues commands and the driver carries them out all the while returning the results.

### Theory
-----
-----
**A driver's** most typical use case is to allow an operating system to talk to a hardware device.A driver doesn't need to control hardware, and pure software drivers do also exist.  The purpose of those will vary - one example of such a driver is anti-virus, where the driver is designed to help protect the computer against malicious actions.

**KPP (aka PatchGuard)** is a feature present in Windows designed to protect the kernel against unauthorised modifications.  It works by periodically checking structures that Microsoft deem sensitive and if a change is detected, it will trigger a bug check and crash the system.When using drivers to circumvent certain kernel-level protection, we are going to stamp over KPP-protected regions.  One "weakness" of KPP is that because the checks are expensive (computationally), it's not constantly checking protected regions.  This introduces a type of race condition where we can modify a protected region and change it back without KPP noticing.

**Protected Processes** were first introduced in Windows Vista - not for security, but DRM (Digital Rights Management).  The idea was to allow media players to read Blu-rays, but not copy the content.  It worked fundamentally by limiting the access you could obtain to a protected process, such as PROCESS_QUERY_LIMITED_INFORMATION or PROCESS_TERMINATE, but not PROCESS_VM_READ or anything else that would allow you to circumvent the DRM requirements.

From the perspective of the kernel, the protection level of a process is stored in a struct called **EPROCESS** - an opaque structure that serves as the process object for a process.  We can obtain a pointer to the EPROCESS struct for a process using PsLookupProcessByProcessId.  But unfortunately, because the struct is opaque, we can't just access its members like **eProcess->Protection**.  Instead, we have to use known offsets.  The downside is that these offsets vary between different versions of Windows.

**Process privilege** determines the type of operations that a process can perform.  A process running in medium integrity has very few privileges available; whereas a process running in high integrity has more.  Some privileges are Default Enabled, which means they are enabled by default .  Others are Disabled but are available, which means they can be enabled using the AdjustTokenPrivileges API. Take SeDebugPrivilege as an example.  The high integrity process has it disabled but available (the token::elevate command in Mimikatz enables this privilege).  The medium integrity process cannot enable it at all.

**Kernel callbacks** provide a way for drivers to receive a notification when certain events occur. These are used rather extensively by AV, EDR and system monitoring applications. The more relevant ones from an attack/defence perspective are:

1. **ProcessNotify** - called when a process is created or exits.  Useful for preventing the process from starting outright, or to inject a userland DLL (that can perform tasks such as API hooking) before control of the process is returned to the caller.

2. **ThreadNotify** - called when a new thread is created or deleted.  Useful for detecting/preventing some process injection techniques by looking for threads being created from one process to another.

3. **LoadImageNotify** - called when a new DLL is mapped into memory.  Useful for detecting/preventing suspicious image loads, such as the CLR being loaded into a native process, or modules synonymous with tools such as Mimikatz.

Since version 10 1607, Windows will not load a kernel-mode driver unless it's signed via the Microsoft Dev Portal.  For developers, this first means obtaining an extended validation (EV) code signing certificate from a provider such as DigiCert, GlobalSign, and others.  They must then apply to join the Windows Hardware Dev Center program by submitting their EV cert and going through a further vetting process.  Assuming they get accepted, a driver needs to be signed by the developer with their EV cert and uploaded to the Dev Portal to be approved and signed by Microsoft. This fairly rigorous process is to protect Windows from malicious and/or unstable code running in the kernel.


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

![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/fadb80e5-cbcb-40a7-8b1c-7fb45d4cccdd)

5. `-r <process no>`    : **Remove callbacks** - This option removes all kernel callbacks issued by the process given.

6. `-ci `                   : **Enumerate DSE** - This option enumerates Driver signature enforcement(DSE) of the driver.


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/1812d277-8990-47e8-86b5-0ee61b4575ed)


  ![image](https://github.com/Swayampadhy/BlueNovember/assets/37104162/291d60f7-e34d-4bae-8eed-e42d4f96127e)

7. `-ciE`                   : **Enable DSE** - This option enables DSE of the driver.

8. `-ciD`                   : **Disable DSE** - This option disables DSE of the driver.


### To-do -
-------
1. Adding more functionalities.
