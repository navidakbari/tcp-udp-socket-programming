# Battle ship implemented using socket programming in c

---
## How to use :

For compiling Server and Client go to their folder and run this command :


    g++ Server.c -o server
    g++ Client.c -o client

* Note: 
You should install gcc compiler first.

Then for run just use this commands in server and client repositories :

      ./Server HeartbeatPort ClientBroadcastPort
      ./Client HeartbeatPort ClientBroadcastPort

## How it works?

If server was alive it broadcast an heartbeat on Heartbeat port. This heartbeat contains IP and PORT that server listening on it for clients to connect. when clients start they listening on heartbeat port an then connect to server. Server job is connecting two clients to each other to play.

If server was not alive when clients did not get anything on Heartbeat port, first they search for others that broadcast their informations to play. If it can not find any client it starts broadcasting it's datas and waiting for someone to connect to it.
