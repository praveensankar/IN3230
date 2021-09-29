#include "queue.h"

int main()
{
    Queue *queue = create_queue();
    int i, choice, input, *data;
    data =  (int *)malloc(sizeof(int));
    while(1)
    {
        printf("enter 1 - enqueue 2 - dequeue 3 - print 4 - exit : ");
        scanf("%d",&choice);
        if(choice==4)
        {
            break;
        }
        switch (choice) {
            case 1:
                printf("enter element : ");
                scanf("%d", &input);
                enqueue(queue, input);
                break;
            case 2:
                if(dequeue(queue, data)!=-1)
                printf("top of the queue is : %d \n ", *data);
                break;
            case 3:
                print_elements(queue);
                break;
        }
    }
    free(data);
    destroy_queue(queue);
    printf("\n");
    return 1;
}