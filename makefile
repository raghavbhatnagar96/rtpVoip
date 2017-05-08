compile:
				gcc client.c -o client -lpulse -lpulse-simple
				gcc server.c -o server -lpulse -lpulse-simple
dependencies:
				sudo apt-get install libpulse0 libpulse-mainloop-glib0 libglib2.0-dev libavahi-client-dev
sendrtp:
				gcc Rtp_Exemple_Send.c RTP.c Hdr_Builder.c Network.c Utils.c -o Rtp_Exemple_Send -lpulse -lpulse-simple
receivertp:
				gcc Rtp_Exemple_Receive.c Rtp_Exemple_Receive_Print.c RTP.c Hdr_Builder.c Network.c Utils.c -o Rtp_Exemple_Receive -lpulse -lpulse-simple
receivertp1:
				gcc Rtp_Exemple_Receive1.c RTP.c Hdr_Builder.c Network.c Utils.c -o Rtp_Exemple_Receive1 -lpulse -lpulse-simple
