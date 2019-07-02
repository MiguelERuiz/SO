#include <unistd.h>
  int main(void)
  {
      write(1,"A\n",2);
      usleep(1500000U);
      write(1,"B\n",2);
      return 0;
  }
