PRJ_NAME = SenSor_Monitoring_System

All:
	gcc -o $(PRJ_NAME) main.c

clean:
	rm -rf $(PRJ_NAME)