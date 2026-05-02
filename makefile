CLIENT_DIR = client
SERVER_DIR = server

c:
	cd $(CLIENT_DIR); make;

cclean:
	cd $(CLIENT_DIR); make clean;

crun:
	cd $(CLIENT_DIR); make run;

s:
	cd $(SERVER_DIR); make;

sclean:
	cd $(SERVER_DIR); make clean;

srun:
	cd $(SERVER_DIR); make run;

clean:
	cd $(CLIENT_DIR); make clean;
	cd $(SERVER_DIR); make clean;