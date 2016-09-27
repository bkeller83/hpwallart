<?php
/**
 * @file Handles the authorization API requests
 */

/**
 * Callback function for path hpwallart/auth
 *
 */
  function hpwallart_authorize() {
   //dpm($_SERVER);
    $all_headers = getallheaders_manual();
    watchdog_array('hpwallart', '$headers from auth.php', $all_headers, WATCHDOG_NOTICE);
    /* We are going to try with just the token based auth
     * So each user will have a unique token based off their user id
     * for now it will just be their user id...
     */
    global $user;
    watchdog_array('hpwallart', '$user from auth.php', (array)$user, WATCHDOG_NOTICE);
    if ($user->hp_auth_token) {
      $auth_token = $user->hp_auth_token;
    } else {
      // Create and save one
      $auth_token = hpwallart_create_token($user->uid);
    }

    drupal_goto($_GET['callback_url'] . '?auth_token=' . $auth_token);
    //header('Location: '.$_GET['callback_url'].'?auth_token='.$auth_token);
   // header('Location: https://designer.hpwallart.com/wallquotes?web_link=true&auth_token='.$auth_token);
    exit;
  }
