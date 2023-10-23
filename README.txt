PART 1: 
Since physical and virtual memory are the same size, we simply copy the physical page and update the page table accordingly. 
Search works via iterating over the table, looking for the physical version over the logical part and returning the physical address if it's found.
Adding works via the same principles, increasing the index and creating a new tblentry.

PART 2:
File was created by copying part 1.
We created a PAGE_FRAMES variable to implement the 256 page solution. We used this variable in FIFO and LRU algorithms to restrict memory space.
We also created a BACKING_SIZE variable to use in creating the backing file.

FIFO algorithm was derived from the first part, with different utilization of PAGE_FRAMES to limit the sizes to 256 bytes.

LRU utilizes an input array of int's to check which page has been accessed recently. We are iterating over the whole referrence table, we believe there could be a more efficient way to do this but we lack the time to optimize this bit of the code.

