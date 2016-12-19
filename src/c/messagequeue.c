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

void message_queue_js_is_ready() {
  js_ready = true;
}

void message_queue_send_next() {
  if (message_queue && js_ready && !sending) {
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