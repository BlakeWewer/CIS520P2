In order to create the alarm-mega test, we had to do a couple things.

The first of which was to modify the tests.c and tests.h files to add the actual alarm-mega test. These were both simple one-line additions into the pre-existing code.

The alarm-wait.c file was where the follow code was inserted:
	void
	test_alarm_mega (void)
	{
  		test_sleep (5, 70);
	}
this was the function that was called in order to run the mega test.

Lastly, the Rubric.alarm file was modified by adding a single line, similar to tests.c and tests.h.

The log-mega.txt file is the output of the alarm-mega test.