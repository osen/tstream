#include "mongoose.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <exception>
#include <vector>

#include <stdio.h>

struct mg_mgr mgr;
struct mg_connection *nc;
struct mg_serve_http_opts http_server_opts;

unsigned char* image1;
size_t size1;
unsigned char* image2;
size_t size2;
unsigned char* image3;
size_t size3;

int curr;

void stbi_func(void *context, void *data, int size)
{
  std::vector<unsigned char>* dat = (std::vector<unsigned char>*)context;
  unsigned char *d = (unsigned char*)data;

  for(size_t i = 0; i < size; i++)
  {
    dat->push_back(d[i]);
  }

  //printf("Here %i \n", (int)size);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      struct mg_str d = {(char *) wm->data, wm->size};
      printf("Received\n");

      if(curr == 0)
      {
        mg_send_websocket_frame(nc, WEBSOCKET_OP_BINARY, image1, size1);
        curr++;
      }
      else if(curr == 1)
      {
        mg_send_websocket_frame(nc, WEBSOCKET_OP_BINARY, image2, size2);
        curr++;
      }
      else if(curr == 2)
      {
        mg_send_websocket_frame(nc, WEBSOCKET_OP_BINARY, image3, size3);
        curr++;
      }
      else if(curr == 3)
      {
        unsigned char data[] = {
          255, 0, 0,
          0, 255, 0,
          0, 0, 255,
          255, 255, 255
        };

        std::vector<unsigned char> dat;
        stbi_write_jpg_to_func(stbi_func, &dat, 2, 2, 3, data, 50);

        mg_send_websocket_frame(nc, WEBSOCKET_OP_BINARY, &dat[0], dat.size());
        curr = 0;
      }

      break;
    }
    case MG_EV_HTTP_REQUEST: {
      mg_serve_http(nc, (struct http_message *) ev_data, http_server_opts);
      break;
    }
  }
}

int main()
{
  FILE* fp = fopen("sample.jpg", "rb");
  if(!fp) throw std::exception();
  fseek(fp, 0L, SEEK_END);
  size1 = ftell(fp);
  printf("Image size1: %i\n", (int)size1);
  rewind(fp);
  image1 = new unsigned char[size1];
  fread(image1, sizeof(*image1), size1, fp);
  fclose(fp);

  fp = fopen("800x600.jpg", "rb");
  if(!fp) throw std::exception();
  fseek(fp, 0L, SEEK_END);
  size2 = ftell(fp);
  printf("Image size2: %i\n", (int)size2);
  rewind(fp);
  image2 = new unsigned char[size2];
  fread(image2, sizeof(*image2), size2, fp);
  fclose(fp);

  fp = fopen("small.jpg", "rb");
  if(!fp) throw std::exception();
  fseek(fp, 0L, SEEK_END);
  size3 = ftell(fp);
  printf("Image size3: %i\n", (int)size3);
  rewind(fp);
  image3 = new unsigned char[size3];
  fread(image3, sizeof(*image3), size3, fp);
  fclose(fp);

  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, "8080", ev_handler);
  mg_set_protocol_http_websocket(nc);
  http_server_opts.document_root = "www";
  http_server_opts.enable_directory_listing = "yes";

  while(true)
  {
    mg_mgr_poll(&mgr, 200);
  }

  mg_mgr_free(&mgr);

  return 0;
}
