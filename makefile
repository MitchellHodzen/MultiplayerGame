CLIENT_DIR = client
SERVER_DIR = server
COMMON_DIR = common

c:
	cd $(COMMON_DIR); make;
	cd $(CLIENT_DIR); make;

cclean:
	cd $(COMMON_DIR); make clean;
	cd $(CLIENT_DIR); make clean;

crun:
	cd $(COMMON_DIR); make;
	cd $(CLIENT_DIR); make run;

s:
	cd $(COMMON_DIR); make;
	cd $(SERVER_DIR); make;

sclean:
	cd $(COMMON_DIR); make clean;
	cd $(SERVER_DIR); make clean;

srun:
	cd $(COMMON_DIR); make;
	cd $(SERVER_DIR); make run;

clean:
	cd $(COMMON_DIR); make clean;
	cd $(CLIENT_DIR); make clean;
	cd $(SERVER_DIR); make clean;