void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly: 
  // CAT_check will look for a command in Yaesu's New Cat format
  // and interpret it. Depending upon the command one of the functions
  // will be called and there we have to put our code for handling 
  // within the arduino sketch
CAT_check();
}
