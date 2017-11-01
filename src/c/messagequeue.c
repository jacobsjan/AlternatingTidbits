#include <pebble.h>
#include "messagequeue.h"

typedef struct MessageQueue MessageQueue;
struct MessageQueue {
  MessageQueue* next;
  int tuplet_count;
  Tuplet tuplets[];
};

static MessageQueue* message_queue = NULL;
bool js_ready = false;
static bool sending = false;

void merge_messages() {
#ifndef PBL_PLATFORM_APLITE
  // Count number of tuplets
  int new_count = 0;
  MessageQueue* old_queue = message_queue;
  while(old_queue) {
    new_count += old_queue->tuplet_count;
    old_queue = old_queue->next;
  }
  
  // Allocate space for all tuplets
  MessageQueue* new_queue = malloc(sizeof(MessageQueue) + new_count * sizeof(Tuplet));
  
  // Merge items
  if (new_queue) {
    new_count = 0;
    old_queue = message_queue;
    while (old_queue) {
      // Merge keys of this message in the old queue
      for (int old_i = 0; old_i < old_queue->tuplet_count; old_i++) {
        // Is this key allready in the new message?
        for (int new_i = 0; new_i < new_count; new_i++) {
          if (old_queue->tuplets[old_i].key == new_queue->tuplets[new_i].key) {
            // Duplicate key, replace with latest value
            memcpy(&new_queue->tuplets[new_i], &old_queue->tuplets[old_i], sizeof(Tuplet));
            break;
          }
        }
        
        // New key, no duplicate found
        memcpy(&new_queue->tuplets[new_count], &old_queue->tuplets[old_i], sizeof(Tuplet));
        new_count += 1;
      }
      
      // On to the next item in the old queue, free previous message
      MessageQueue* freeable = old_queue;
      old_queue = old_queue->next;
      free(freeable);
    }
    
    new_queue->tuplet_count = new_count;
    new_queue->next = NULL;   
      
    // Replace with new queue
    message_queue = new_queue;
  }
#endif
}

void message_queue_js_is_ready() {
  js_ready = true;
}

void message_queue_send_next() {
  if (message_queue && js_ready && !sending) {
    // Merge messages into one
    if (message_queue->next) merge_messages();
    
    // Prepare dictionary to send
    DictionaryIterator *iter;
    if(app_message_outbox_begin(&iter) == APP_MSG_OK) {
      sending = true;
    
      for (int n = 0; n < message_queue->tuplet_count; ++n) {
        dict_write_tuplet(iter, &message_queue->tuplets[n]);
      }
      dict_write_end(iter);
      
      app_message_outbox_send();
    } 
  }
}

void message_queue_sent_handler(DictionaryIterator *iter, void *context) {
  sending = false;
  
  // Free sent message
  MessageQueue* sent = message_queue;
  message_queue = message_queue->next;
  free(sent);
  
  // Send next message
  message_queue_send_next();
}

void message_queue_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  sending = false;
}

void message_queue_send(Tuplet tuplets[], int tuplet_count) {
  MessageQueue* queue = malloc(sizeof(MessageQueue) + tuplet_count * sizeof(Tuplet));
  if (queue) {
    queue->next = NULL;
    queue->tuplet_count = tuplet_count;
    memcpy(queue->tuplets, tuplets, tuplet_count * sizeof(Tuplet));
    
    if (!message_queue) {
      message_queue = queue;
    } else {
      MessageQueue* list = message_queue;
      while(list->next) list = list->next;
      list->next = queue;
    }
    
    message_queue_send_next();
  }
}

void message_queue_send_tuplet(Tuplet tuplet) {
  message_queue_send((Tuplet[]){ tuplet }, 1);
}

void message_queue_deinit() {
  MessageQueue* queue = message_queue;
  while (queue) {
    MessageQueue* freeable = queue;
    queue = queue->next;
    free(freeable);
  }
  message_queue = NULL;
}