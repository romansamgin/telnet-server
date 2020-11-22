# telnet-server

Запустить сервер с аргументом порта :

  gcc telnet_server.c -o server
  
  ./server 50001    //или любое число из 49152—65535
  
  telnet localhost 50001
  
  ls, pwd, ls -l | wc -l и тд
