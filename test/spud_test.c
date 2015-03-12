/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <check.h>

#include "tube.h"
#include "spud.h"
#include "ls_error.h"
#include "ls_sockaddr.h"

void spudlib_setup (void);
void spudlib_teardown (void);
Suite * spudlib_suite (void);

void
spudlib_setup (void)
{

}

void
spudlib_teardown (void)
{

}

START_TEST (empty)
{

    fail_unless( 1==1,
                 "Test app fails");

}
END_TEST

START_TEST (is_spud)
{
    int len = 1024;
    unsigned char buf[len];

    spud_header hdr;
    //should this be in init() instead?
    memcpy(hdr.magic, SpudMagicCookie, SPUD_MAGIC_COOKIE_SIZE);

    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless( spud_is_spud((const uint8_t *)&buf,len),
                 "isSpud() failed");
}
END_TEST

START_TEST (createId)
{
    int len = 1024;
    unsigned char buf[len];
    char idStr[len];
    ls_err err;

    spud_header hdr;
    //should this be in init() instead?
    fail_unless(spud_init(&hdr, NULL, &err));

    printf("ID: %s\n", spud_ido_string(idStr, len, &hdr.tube_id, NULL));

    fail_if(spud_is_spud((const uint8_t *)&buf,len),
            "isSpud() failed");


    //copy the whole spud msg into the buffer..
    memcpy(buf, &hdr, sizeof(hdr));

    fail_unless(spud_is_spud((const uint8_t *)&buf,len),
                "isSpud() failed");
}
END_TEST

START_TEST (isIdEqual)
{
    ls_err err;

    spud_header msgA;
    spud_header msgB;//Equal to A
    spud_header msgC;//New random

    fail_unless(spud_init(&msgA, NULL, &err));
    fail_unless(spud_init(&msgB, &msgA.tube_id, &err));
    fail_unless(spud_init(&msgC, NULL, &err));

    fail_unless( spud_is_id_equal(&msgA.tube_id, &msgB.tube_id));
    fail_if( spud_is_id_equal(&msgA.tube_id, &msgC.tube_id));
    fail_if( spud_is_id_equal(&msgB.tube_id, &msgC.tube_id));
}
END_TEST

START_TEST (spud_parseest)
{


}
END_TEST

START_TEST (tube_create_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    ls_event_dispatcher *dispatcher;
    fail_unless( ls_event_dispatcher_create(&t, &dispatcher, &err) );
    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_create(sockfd, dispatcher, &t, &err) );

}
END_TEST

START_TEST (tube_destroy_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    tube_destroy(t);
}
END_TEST

static void running_cb(ls_event_data evt, void *arg){}
static void close_cb(ls_event_data evt, void *arg){}
static void data_cb(ls_event_data evt, void *arg){}

START_TEST (tube_bind_events_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    char arg[12];
    ls_event_dispatcher *dispatcher;

    fail_unless( ls_event_dispatcher_create(&t, &dispatcher, &err) );

    fail_unless( tube_bind_events(dispatcher,
                                  running_cb,
                                  data_cb,
                                  close_cb,
                                  arg,
                                  &err), ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_print_test)
{
    tube *t;
    int sockfd;
    ls_err err;

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_print(t, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_open_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_ack_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );
    fail_unless( tube_ack(t,
                          &t->id,
                          (const struct sockaddr*)&remoteAddr,
                          &err),
                 ls_err_message( err.code ) );
}
END_TEST

START_TEST (tube_data_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    uint8_t data[] = "SPUD_makeUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_data(t,
                           data,
                           17,
                           &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_data(t,
                           NULL,
                           0,
                           &err),
                 ls_err_message( err.code ) );

}
END_TEST

START_TEST (tube_close_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    char data[] = "SPUD_makeUBES_FUN";
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( tube_close(t,
                            &err),
                 ls_err_message( err.code ) );

}
END_TEST

START_TEST (tube_recv_test)
{
    tube *t;
    int sockfd;
    ls_err err;
    char data[] = "SPUD_makeUBES_FUN";
    spud_message sMsg = {NULL, NULL};
    spud_header smh;
    struct sockaddr_in6 remoteAddr;
    fail_unless( ls_sockaddr_get_remote_ip_addr(&remoteAddr,
                                                "1.2.3.4",
                                                "1402",
                                                &err),
                 ls_err_message( err.code ) );

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

    fail_unless( tube_create(sockfd, NULL, &t, &err) );

    fail_unless( tube_open(t, (const struct sockaddr*)&remoteAddr, &err),
                 ls_err_message( err.code ) );

    fail_unless( spud_init( &smh, &t->id, &err) );

    smh.flags = 0;
    sMsg.header = &smh;
    t->state = TS_RUNNING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

    smh.flags |= SPUD_OPEN;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

    smh.flags |= SPUD_ACK;
    t->state = TS_OPENING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );


    smh.flags |= SPUD_CLOSE;
    t->state = TS_RUNNING;
    fail_unless( tube_recv(t,
                           &sMsg,
                           (const struct sockaddr*)&remoteAddr,
                           &err),
                 ls_err_message( err.code ) );

}
END_TEST



Suite * spudlib_suite (void)
{
  Suite *s = suite_create ("spudlib");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_checked_fixture (tc_core, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_core, empty);
      tcase_add_test (tc_core, is_spud);
      tcase_add_test (tc_core, createId);
      tcase_add_test (tc_core, isIdEqual);

      suite_add_tcase (s, tc_core);
  }

  {/* tube *test case */
      TCase *tc_tube = tcase_create ("TUBE");
      tcase_add_checked_fixture (tc_tube, spudlib_setup, spudlib_teardown);
      tcase_add_test (tc_tube, tube_create_test);
      tcase_add_test (tc_tube, tube_destroy_test);
      //tcase_add_test (tc_tube, tube_bind_events_test);
      //tcase_add_test (tc_tube, tube_print_test);
      //tcase_add_test (tc_tube, tube_open_test);
      //tcase_add_test (tc_tube, tube_ack_test);
      //tcase_add_test (tc_tube, tube_data_test);
      //tcase_add_test (tc_tube, tube_close_test);
      tcase_add_test (tc_tube, tube_recv_test);

      suite_add_tcase (s, tc_tube);
  }


  return s;
}
