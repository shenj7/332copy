ex:
	gcc preempt_example.c -o example

clean:
	rm ./standalone1

sa1:
	rm ./standalone1
	gcc standalone1.c preempt_threads.c -o standalone1

sa2:
	rm ./standalone2
	gcc standalone2.c preempt_threads.c -o standalone2

preempt:
	rm preempt_tests
	gcc preempt_tests.c preempt_threads.c CuTest.c -o preempt_tests
