# callblocker
Blocks unwanted calls from your home phone, such as telemarketing, junk, spam, scam, etc.

The call blocker acts like a normal phone. No additional telephone switchboard (like FreePBX or Asterisk) is needed. When a telemarketer is calling, the call blocker picks up the phone and automatically hangs up after a few seconds. Like this the call is blocked. It's designed to run on small footprints such as a Raspberry Pi. In addition when the call blocker is not running (e.g. hardware broke) your home phone is still working. It has the ability to check against online and offline spam lists.


## Features
- automatically blocks unwanted incoming calls
- logging of all incoming calls, including caller name lookups
- different kind of blocking modes
  - logging only
  - number blocking using blacklists only
  - number blocking using blacklists and avoid blocking, if number is in whitelist
  - number has to be in whitelist, all other numbers are blocked
- different kind of ways to verify incoming calls
  - user maintained offline blacklist
  - extern maintained offline blacklists (downloaded from the Internet and stored offline)
  - support of online spam check sites, for spam verification
- import your addressbook as whitelist or blacklist
- using whitelists and blacklists as offline lookup for caller name
- support of online lookup sites, to find out who is calling
- nice web interface


## Hardware
Supported (tested) server host systems
- Raspberry Pi (running raspbian/jessie)

Supported (tested) VoIP systems
- Fritzbox 7390
- NetVoip.ch

Supported (tested) analog modems
- USRobotics 5637


## Install daemon on a Raspberry Pi (running raspbian/jessie)
```bash
sudo apt-get install libboost-dev libboost-regex-dev
sudo apt-get install python python-beautifulsoup python-demjson python-ldap python-vobject
cd /usr
git clone https://github.com/buffi79/callblocker.git
cd /usr/callblocker/configs
mv tpl_settings.json settings.json
vi settings.json
sudo systemctl start callblockerd.service

sudo journalctl _SYSTEMD_UNIT=callblockerd.service
```
## For source compilation
```bash
sudo apt-get install git automake g++ libpjproject-dev libjson-c-dev libboost-dev libboost-regex-dev 
cd /usr/callblocker/src/
aclocal
automake --add-missing --foreign
autoconf
./configure --prefix=/usr/callblocker/ --sysconfdir=/usr
make all
sudo make install
```

## <a name="webInterface"></a> Install web interface on a Raspberry Pi (running raspbian/jessie)
```bash
sudo apt-get install lighttpd python-flup libjs-dojo-core libjs-dojo-dijit libjs-dojo-dojox
sudo chgrp -R www-data /usr/callblocker/www/
sudo usermod -a -G systemd-journal www-data
sudo chmod a+x /usr/callblocker/www/callblocker/python-fcgi/api.py
sudo vi /etc/lighttpd/lighttpd.conf
sudo ln -s /usr/callblocker/www/callblocker/ /var/www/
```
1. In the upper section of this file you can find the section 'server.modules='. Please add the module "mod_fastcgi".
2. 
```section
alias.url = (
 "/javascript/" => "/usr/share/javascript/"
)

$HTTP["url"] =~ "^/callblocker($|/)" {
  fastcgi.server              = (
        ".py" => (
                "callblocker-fcgi" => (
                        "bin-path" => "/usr/callblocker/www/callblocker/python-fcgi/api.py",
                        "socket" => "/var/run/lighttpd/fastcgi.python.socket")
        )
  )
}
```
4. Make sure the python file fcgi_api.py has correct executable rights and restart lighttpd daemon.
```bash
sudo chmod a+x /usr/callblocker/www/callblocker/python-fcgi/api.py
sudo systemctl restart lighttpd.service

sudo cat /var/log/lighttpd/error.log
sudo journalctl -xn _SYSTEMD_UNIT=lighttpd.service
```
For additional information see [here](http://redmine.lighttpd.net/projects/lighttpd/wiki/Docs_ModFastCGI).


## <a name="fileLayout"></a> File Layout
When installed on Linux, the following file layout is used
```
/usr/callblocker                           #homedirectory of callblocker
                /bin/callblockerd          # daemon
                /configs                   # config-Files
                        /blacklists        # put your blacklists here
                        /whitelists        # put your whitelists here
                /scripts                   # python helper scripts            
                /www/callblocker           # web interface
                /src                       # C++ Source callblockerdeamon
```

## <a name="settingsJson"></a> Configuration file
The documentation of the configuration file "settings.json" is located [here](/configs/callblocker/README.md).


## Offline blacklists (automatically periodically downloading)
Through the web interface you have the possibility to maintain your own blacklist. Additionally there is the
possibility to periodically download an extern maintained blacklist. You will need to setup a cronjob for this task.

Currently the following blacklists are supported:

Name                         | Site                       | Description
----                         | ----                       | -----------
blacklist_toastedspam_com.py | http://www.toastedspam.com | Mostly USA and Canada (+1)
blacklist_ktipp_ch.py        | https://www.ktipp.ch       | Switzerland (+41)

The following cronjob will download each day the blacklist provided by ktipp_ch:
```bash
0 0 * * * /usr/callblocker/scripts/blacklist_ktipp_ch.py --output /usr/callblocker/configs/blacklists/ >/dev/null 2>&1
```


## Setup
There are two ways to connect the call blocker application with your phone system, depending if it is VoIP or analog. 


### Setup using Fritzbox with a IP-phone
- Create in the Fritzbox a new IP-phone
  - Open your web browser and navigate to the URL http://fritz.box
  - In the menu "Telefonie -> Telefoniegeraete" click on "Neues Geraet einrichten"
  - Choose "Telefon (mit und ohne Anrufbeantworter)" and click "Weiter"
  - Choose "LAN/WLAN (IP-Telefon)", for name use for example "callblocker" and click "Weiter"
  - Choose a password, remember it and click "Weiter"
  - Choose "alle Anrufe annehmen" and click "Weiter"
- Setup the IP-phone in the call blocker configuration (/usr/callblocker/configs/setting.json):
  - Edit the section sip -> accounts
  - "from_domain":   "fritz.box"
  - "from_username": "your username"
  - "from_password": "your password"
  - Make sure the account is enabled and the other fields ok ok for you


### Setup using an analog phone
- Attach the USB modem to the Raspberry Pi
- Use `dmesg` to find the device name `/dev/<name>`
- Setup the Analog phone in the call blocker configuration (/usr/callblocker/configs/setting.json):
  - Edit the section analog -> phones
  - "device": "your device name"
  - Make sure the account is enabled and the other fields ok ok for you


## Troubleshooting

### Symptom: It is unspecific not working.
1. Double check all installed files, with its locations and permissions. See [file layout](#fileLayout)
2. Make sure lighttpd and callblockerd are running.<br>
   ```bash
   sudo ps aux | grep -E 'lighttpd|callblockerd' | grep -v 'grep' # shows: 2 lines
   ```
3. Check for possible errors/warning.<br>
   ```bash
   sudo journalctl _SYSTEMD_UNIT=callblockerd.service
   ```
4. Increase log levels: "log_level" to "debug" and/or "pjsip_log_level" to 2. See documentation of
   [configuration file](#settingsJson) for more info.
   ```bash
   vi settings.json
   ```

### Symptom: Web interface is not working.
The web interface is running within lighttpd, double check the [web configuration](#webInterface) of this deamon. Also
look into the seperate log file:
```bash
sudo cat /var/log/lighttpd/error.log
sudo journalctl -xn _SYSTEMD_UNIT=lighttpd.service
```

### Symptom: Configuration done within the web interface is not saved persistent.
The web interface is running within lighttpd, this deamon is using "www-data" as user and group. Make
sure that this process has access to the configuration file (see [file layout](#fileLayout)).
```bash
sudo chgrp -R www-data /usr/callblocker/www/
```

### Symptom: Caller log and diagnostics stay empty within the web interface.
Make sure journal is active and working and the web interface has access to the journal.The web interface
depends on functionality provided by systemd journal. 
```bash
# switch to systemd journal
sudo apt-get purge rsyslog logrotate libestr0 liblogging-stdlog0 liblognorm1
sudo vi /etc/systemd/journald.conf: #Storage=auto -> Storage=auto
sudo rm -rf /var/log/* # optional, you will lose all existing log entries (old format)
sudo mkdir /var/log/journal
sudo reboot # required to finished the switch

# allow web interface access the journal
sudo usermod -a -G systemd-journal www-data
sudo systemctl restart lighttpd.service

# manual verify that journal is working
sudo journalctl _SYSTEMD_UNIT=callblockerd.service
```

