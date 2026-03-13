/opt/pmb100-terminal/RUNSD                   (questo file va cancellato in fase di copia della SD su MMC  , viene rigenerato da terminal.bin)

/etc/modules-load.d/modules.conf             (commentato #mtdblock  per non montare filesystem MRAM)
systemctl disable data-mram.service          (disabilitato il servizio mram)
/etc/systemd/system/pmb100-terminal.service  (commento le due righe per togliere vincolo MRAM e inserisco vincolo eth0)
Requires=sys-subsystem-net-devices-eth0.device
After=sys-subsystem-net-devices-eth0.device
#Wants=data-mram.service
#After=data-mram.service

/etc/CODESYSControl.cfg                      ( modificato per LOG su /tmp)
[CmpLog]
Logger.0.Name=/tmp/codesyscontrol.log

/etc/CODESYSControl_User.cfg                 (modificato per RETAIN )
[CmpRetain]
Retain.SHM.Size=0x10100        ; 65792 byte Total size of permanent memory in bytes su diso SD/MMC
Retain.SHM.Name=MyRetainMemory ; Optional (Default: "RetainMemory")

/opt/codesys/scripts/InstallUsbMount.sh      (comentare il contenuto)
/opt/codesys/scripts/index.bin               (da cancellare non serve piu)
/opt/codesys/scripts/rts_set_baud.sh         (questo file va modificato per fissare il baudrate a 1M in caso di CAN0)
#!/bin/sh
BITRATE=`expr $2 \\* 1000`
FILE=/tmp/PLE500present
ip link set $1 down
if [ "$1" = "$CAN0" ]
then
        if [ -f "$FILE" ]; then
            #echo "$FILE exists."
            ip link set can0 type can bitrate 1000000 restart-ms 100
            ip link set can0 up
        else
            ip link set can0 type can bitrate 1000000 restart-ms 100
            ip link set can0 down
        fi
else
        ip link set $1 type can bitrate $BITRATE
        ip link set $1 up
fi

/etc/systemd/system/codesyscontrol.service  (servizio codesys deve partire in Stop / Disable viene lanciato da Pmb100-terminal.bin )
/etc/init.d/codesyscontrol                  (old metodo di autorun viene chiamata da systemd , se chiama con start o stop avvia o spegne codesys runtime )
/opt/codesys/scripts/init-functions         (modificare before_start_runtime () per retain da shared memory)
retain_runtime () {
    touch /data/MyRetainMemory
    touch /dev/shm/MyRetainMemory
    /bin/mount -o bind,sync /data/MyRetainMemory /dev/shm/MyRetainMemory
    echo 'start ' $(date) >> /data/mounttime.txt
}
before_start_runtime () {
    create_nodes
    load_modules
    fix_permissions
    retain_runtime
}
after_stop_runtime () {
     /bin/unmount /data/MyRetainMemory
     echo 'stop ' $(date) >> /data/mounttime.txt
}

************************** COMPILAZIONE STATICA ********
inserire su campo  MSbuild settings / aditional linker settings  "-static"

************************** MODIFICHE HARDWARE  CPU *************************************
Togliere condensatore su MASSA  verso  TERRA su alimentazione e cortocircuitarlo , senza questa modifica il connettore USB non funziona corettamente sotto disturbi ,
fatica a tenere 400V su alimentazione , con questa modifica lo schermo delll'USB viene collegato a massa e tutto funziona meglio , arriva  quasi a 1000V.
condensatore RTC errato corre troppo
systemare su RTC condensatore per precisione

************************** MODIFICHE LATERALE ALIMENTAZIONE *************************
togliere condensatore rampa su maxim 5V C28
togliere resistenza shunt per caricare condensatore backup a 250ma R34
condensatori UPS da 1F  c25 e c23
