#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>	
#include <unistd.h>
#include <stdbool.h>
#include <gtk/gtk.h>	//gui library

#define NUM_PHILOSOPHERS 5	//total philosphers
#define THINKING 0	//philophers state
#define HUNGRY 1	//philophers state
#define EATING 2	//philophers state

int eatArr[5]={0,0,0,0,0};
int state[NUM_PHILOSOPHERS];
bool stop=false;
sem_t mutex;	//for mutual exclusion while changing state of philosophers
sem_t semaphores[NUM_PHILOSOPHERS];	//to keep track of which fork is available
sem_t updatelabels;
sem_t updatetext;
GtkWidget *text_view;
GtkWidget *phil_labels[NUM_PHILOSOPHERS];	//to display philosophers states
GtkWidget *eat_labels[NUM_PHILOSOPHERS];
GtkWidget *grid;

int LEFT(int philosopher_number)
{
	return 	(philosopher_number + NUM_PHILOSOPHERS - 1) % NUM_PHILOSOPHERS;
}

int RIGHT(int philosopher_number)
{
	return (philosopher_number ) % NUM_PHILOSOPHERS;
}
static gboolean append_to_text_view(gpointer data) 
{
	sem_wait(&updatetext);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_buffer_insert(buffer, &end, (char *)data, -1);
	g_free(data);  //  free the allocated string
	sem_post(&updatetext);
	return FALSE; // Return FALSE as we don't need to call this function again	
}

void update_philosopher_label(int phnum,gchar *new_text) 
{
	sem_wait(&updatelabels);
	gtk_label_set_text(GTK_LABEL(phil_labels[phnum]), new_text);
	
	g_free(new_text);
	sem_post(&updatelabels);
}

void update_eat_label(int phnum,gchar *new_text) 
{
	gtk_label_set_text(GTK_LABEL(eat_labels[phnum]), new_text);
	g_free(new_text);
}

void stopFunc(GtkWidget *button, gpointer data)
{
	stop=true;
	return;
}
// Function to pick up forks
void pickup_forks(int philosopher_number) 
{
	sem_wait(&mutex);
	state[philosopher_number] = HUNGRY;
    	gchar *message = g_strdup_printf("Philosopher %d is hungry\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
        update_philosopher_label(philosopher_number, g_strdup_printf("P%d: Hungry", philosopher_number+1));    
	sem_post(&mutex);
    
	sem_wait(&semaphores[RIGHT(philosopher_number)]);
	message = g_strdup_printf("Philosopher %d picked up the right fork\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
    	
    	// Wait for right fork
    	sem_wait(&semaphores[LEFT(philosopher_number)]);
    	message = g_strdup_printf("Philosopher %d picked up the left fork\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
	sem_wait(&mutex);
	state[philosopher_number] = EATING;
	sem_post(&mutex);
    	
}

// Function to return forks
void return_forks(int philosopher_number) 
{
	sem_post(&semaphores[RIGHT(philosopher_number)]);
	gchar *message = g_strdup_printf("Philosopher %d returned the right fork\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
        
    	sem_post(&semaphores[LEFT(philosopher_number)]);
    	message = g_strdup_printf("Philosopher %d returned the left fork\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
        update_philosopher_label(philosopher_number, g_strdup_printf("P%d: Thinking", philosopher_number+1));
    	
	sem_wait(&mutex);
	state[philosopher_number] = THINKING;
	sem_post(&mutex);
    	
}

// Philosopher thread function
void* philosopher(void* arg) 
{
    int philosopher_number = *((int*)arg);
    while(!stop) 
    {
    	// Think
        gchar *message = g_strdup_printf("Philosopher %d is thinking\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
        update_philosopher_label(philosopher_number, g_strdup_printf("P%d: Thinking", philosopher_number+1));
        
        sleep(2);

        // Pick up forks
        pickup_forks(philosopher_number);
        message = g_strdup_printf("Philosopher %d is eating\n", philosopher_number+1);
        gdk_threads_add_idle(append_to_text_view, message);
        update_philosopher_label(philosopher_number, g_strdup_printf("P%d: Eating", philosopher_number+1));        
        //update philosopher eat count
        eatArr[philosopher_number]+=1;	
        
        sleep(2);

        // Return forks
        return_forks(philosopher_number);
        //displaying eat count
        for(int i=0;i<5;i++)
        {
        	printf("P%d : %d\n",i+1,eatArr[i]); 
        }
        printf("\n");
        
    }
}

int main(int argc,char *argv[]) 
{
	gtk_init(&argc, &argv);
	GtkWidget *window=gtk_window_new(GTK_WINDOW_TOPLEVEL);	//opening window
	gtk_window_set_title(GTK_WINDOW(window), "Dining Philosophers Project");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);	//exit the program if window is closed
	
	// Create a horizontal box container
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	//adding hbox in window
	gtk_container_add(GTK_CONTAINER(window), hbox);
	
	
	// Create scrolled window
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	//adding scrolled window in hbox
	gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 0);
	
	// Create text view
	text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	
	// Create image
    	GtkWidget *image = gtk_image_new_from_file("diningimage.png"); 	
	//creating grid
	grid=gtk_grid_new();
	//adding grid in hbox
        gtk_box_pack_start(GTK_BOX(hbox), grid, FALSE, FALSE, 0);
        	
  	for (int i = 0; i < NUM_PHILOSOPHERS; i++) 
  	{
        	phil_labels[i] = gtk_label_new("");
        	
    	}
    	
    	GtkWidget *button = gtk_button_new_with_label("STOP");
    	g_signal_connect(button, "clicked", G_CALLBACK(stopFunc), NULL);
    	
    	//adding a stop button in grid
    	gtk_grid_attach(GTK_GRID(grid), button, 1, 1, 1, 1);
	
	// Add philosopher labels and image in the grid
    	gtk_grid_attach(GTK_GRID(grid), phil_labels[0], 900, 800, 1, 1);
    	gtk_grid_attach(GTK_GRID(grid), phil_labels[1], 1500, 1000, 1, 1);
    	gtk_grid_attach(GTK_GRID(grid), phil_labels[2], 1500, 1100, 1, 1);
    	gtk_grid_attach(GTK_GRID(grid), phil_labels[3], 650, 1100, 1, 1);
    	gtk_grid_attach(GTK_GRID(grid), phil_labels[4], 650, 1000, 1, 1);
    	gtk_grid_attach(GTK_GRID(grid), image, 900, 1000, 1, 1);
    	
    	//showing window
    	gtk_widget_show_all(window);
    	
	//initializing semaphores
	sem_init(&mutex, 0, 1);	
	sem_init(&updatelabels, 0, 1);	
	sem_init(&updatetext, 0, 1);	
	
	for (int i = 0; i < NUM_PHILOSOPHERS; i++)
	{
        	sem_init(&semaphores[i], 0, 1);
    	}
	
	// Create philosopher threads having start function of philosopher
	pthread_t philosophers[NUM_PHILOSOPHERS];
	int philosopher_numbers[NUM_PHILOSOPHERS];
	for (int i = 0; i < NUM_PHILOSOPHERS; i++) 
	{
		philosopher_numbers[i] = i;
		pthread_create(&philosophers[i], NULL, philosopher, &philosopher_numbers[i]);
	}
	//start gtk main loop
	gtk_main();
	// Join philosopher threads only if stop button on window is pressed
	if(stop)
	{
		for (int i = 0; i < NUM_PHILOSOPHERS; i++) 
		{
			pthread_join(philosophers[i], NULL);
		}
	}
	// Destroy semaphores
	sem_destroy(&mutex);
	sem_destroy(&updatelabels);
	sem_destroy(&updatetext);	
	for (int i = 0; i < NUM_PHILOSOPHERS; i++) 
	{
		sem_destroy(&semaphores[i]);
	}

	return 0;
}
