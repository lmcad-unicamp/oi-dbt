int main()
{
  int BUBBLE = 100;
  int myArray[BUBBLE];
  int i, j;
  int temp = 0;
  int num;

  for (i = 0; i < BUBBLE; i ++) {
    num = i*4 % BUBBLE;
    myArray[i] = num;
  }

  for(i = 0; i < BUBBLE; i++)	{
    for (j = 0; j < BUBBLE-1; j++) {
      if (myArray[j] > myArray[j+1]) {
        temp = myArray[j];
        myArray[j] = myArray[j+1];
        myArray[j+1] = temp;
      }
    }
  }

  return myArray[10] + myArray[13];
}
