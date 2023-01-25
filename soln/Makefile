run:
		gnome-terminal -- ./server/server
		gnome-terminal -- python3 ./client/client.py

clean:
		rm inpipe outpipe
		cd server && $(MAKE) clean
