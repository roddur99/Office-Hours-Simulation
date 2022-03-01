# Office-Hours-Simulation
A multi-threaded application that simulates a professor's office hours between students from two classes (A and B)

Language: C

The simulation follows the following conditions:
- The professors's office has only 3 seats, so no more than 3 students are allowed to simultaneously enter the professor’s office. When the office is full and new 
students arrive they have to wait outside the office.
- After seeing 10 consecutive students, the professor takes a break.
- After seeing 5 consecutive students from one class, the professor switches to another class
- No students are forced to wait when professor is not taking a break and not seeing other students.
