<?php
	/**
   * https://designer.newui.hpwallart.com/wallquotes?auth_token=2
   * https://designer.hpwallart.com/wallquotes?web_link=true?auth_token=2 ?sku=WA_WC wall covering // ?sku=WA_WD wall decal
	*/


/**
 * If things work right, this function should not get called.
 * We are handling all the legitimate requests through their own hook menu
 * functions. So any other requests with this default path are invalid.
 */
function hpwallart_services() {
  $all_headers = getallheaders_manual();
  watchdog_array('hpwallart', 'Invalid URL Request at /services', $all_headers, WATCHDOG_ERROR);
  global $base_url;
  drupal_goto($base_url);
  exit;
}

/**
 * Handles the services User Info API request
 *  This API gives HP WallArt the id and name of the user signed into the E-Commerce site.
 */
function hpwallart_services_user_info() {
  $user_token = $_SERVER['HTTP_X_AUTH_TOKEN'];
  $user_id = hpwallart_get_user_id_by_token($user_token);
  $user = user_load($user_id);
  $user_info = array(
      'id' => $user->uid,
      'name' => $user->name,
      );
	echo json_encode($user_info);
  watchdog_array('hpwallart', '$user_info from services_user_info', json_encode($user_info), WATCHDOG_NOTICE);
}

/**
 * Handles the projects API requests
 */
function hpwallart_services_projects() {

  $all_headers = getallheaders_manual(); // this just removes the HTTP from var names like HTTP_X_AUTH_TOKEN and makes stuff lower case
  watchdog_array('hpwallart', '$headers from services_projects', $all_headers, WATCHDOG_NOTICE);
  $user_token = $all_headers['X-Auth-Token'];
  $user_id = hpwallart_get_user_id_by_token($user_token);
  $user = user_load($user_id);

// watchdog_array('hpwallart', '$_SERVER from services_projects', $_SERVER, WATCHDOG_NOTICE);
// watchdog_array('hpwallart', '$_REQUEST from services_projects', $_REQUEST, WATCHDOG_NOTICE); THIS IS EMPTY

  //first the case where we send the list of projects in an array
    if ($all_headers['REQUEST_METHOD'] == "GET") {
      /* This API gives HP WallArt Designer the list of projects already created by the user signed into the E-Commerce site. */
      // get a list of projects for this user and echo
      $projects = hpwallart_get_user_projects($user->uid);
      // not sure if providing ALL the project info will cause an error...
      // API asks for id, name, path, content_context_token, state
      watchdog_array('hpwallart', '$projects=get', $projects, WATCHDOG_NOTICE);
      echo json_encode($projects);
      exit;
    }
    //second the case where we receive data from a project.
    elseif ($all_headers['REQUEST_METHOD'] == "POST") {
      /** This API will let HP WallArt Designer create a new project in the folder
       * specified by "path" and return the projects id in the E Commerce Sites
       * database. This project should be owned by the user signed into the E-Commerce site.
       * Note that $data is an object, not an array!!!
       * **/

      $data = json_decode(file_get_contents("php://input")); // supplies name, path, and content token?
      watchdog_array('hpwallart', '$data project=post', $data, WATCHDOG_NOTICE);

      $project = new HPWallArtProject($data);
      watchdog_array('hpwallart', '$project line 122', $project, WATCHDOG_NOTICE);
      // need to see if there is an id present here!
      if ($project->getId() === NULL) {
        $project->setId($project->save_new_project($user_id));
      }  // else save here?
      $project->json_echo();

      exit;
    }


}

/**
 * Handles the projects/id API requests
 */
function hpwallart_services_projects_id($id) {
  $all_headers = getallheaders_manual(); // this just removes the HTTP from var names like HTTP_X_AUTH_TOKEN and makes stuff lower case
  watchdog_array('hpwallart', '$headers from services_projects_id = '.$id, $all_headers, WATCHDOG_NOTICE);

  $user_token = $all_headers['X-Auth-Token'];
  $user_id = hpwallart_get_user_id_by_token($user_token);
  $user = user_load($user_id);

  if ($all_headers['REQUEST_METHOD'] == "GET") {
    // This API gives HP WallArt Designer data regarding a project specified by <id> already created by the user signed into the E-Commerce site.
    $project = hpwallart_get_project_by_id($id);
    $project->json_echo();
    exit;

  } elseif ($all_headers['REQUEST_METHOD'] == "PUT") {
    $data = json_decode(file_get_contents("php://input")); // supplies name, path, and content token?
    watchdog_array('hpwallart', '$data project=put', $data, WATCHDOG_NOTICE);

    $project = new HPWallArtProject($data);
    watchdog_array('hpwallart', '$project line 153', $project, WATCHDOG_NOTICE);
    // Update this project
    $project->save_project();
    $project->json_echo();
  }


}

/**
 * Handles the project add to cart API request
 * @param type $project_id
 * Adds a project to the customer's cart.
 * Product attributes - price, hp order id, project id, project name, hp sku, images
 * uc sku = hp-wallart/dev nid = 2124/
 * @todo do we need to update the status in projects?
 */
function hpwallart_services_projects_add_to_cart($project_id) {
  $all_headers = getallheaders_manual(); // this just removes the HTTP from var names like HTTP_X_AUTH_TOKEN and makes stuff lower case
  watchdog_array('hpwallart', '$headers from project_add_to_cart id = '.$project_id, $all_headers, WATCHDOG_NOTICE);

  $p = file_get_contents('https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/'.$project_id);
  $project = json_decode($p);
  $hp_order_id = $project->order_id; // int
  $name = $project->name; // Project name entered by customer
  $price = round( (float) $project->price->base->price, 2 );
  $sku = $project->product_descriptor->product_sku; // WA_WD
  $sku_array = array('WA_WP' => 'Poster', 'WA_WC' => 'Wall', 'WA_CA' => 'Canvas', 'WA_WD' => 'Decal',);
  $img_small = 'https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/' . $project_id . '/preview_small.png'; // 180px
  $img_medium = 'https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/' . $project_id . '/preview_medium.png'; // 400px
  $img_large = 'https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/' . $project_id . '/preview_large.png'; // 800px


  $ats = array('nid' => 2124, 'qty' => 1, 'data' => array(
    'varprice' => $price,
    'attributes' => array(
      1734 => $name,
      1735 => $hp_order_id,
      1736 => $project_id,
      1737 => $sku_array[$sku],
      1738 => $img_large,
      ))
    );
 // uc_cart_add_item($nid, $qty = 1, $data = NULL, $cid = NULL, $msg = TRUE, $check_redirect = TRUE, $rebuild = TRUE);
  uc_cart_add_item(2124, 1, $ats['data'] + module_invoke_all('uc_add_to_cart_data', $ats), NULL, NULL, FALSE, FALSE);
  // Update the project and project state to IN_CART
  $project = new HPWallArtProject($project);
  $project->setState('IN_CART');
  $project->save_project();
  global $base_url;
  drupal_goto($base_url . '/cart');
}

/**
 * Handles the Multiple Projects add to to cart API request
 * @todo this needs built still
 */
function hpwallart_services_multi_projects_add_to_cart() {
  $project_ids = check_plain($_POST['project_ids']);
  $all_headers = getallheaders_manual(); // this just removes the HTTP from var names like HTTP_X_AUTH_TOKEN and makes stuff lower case
  watchdog_array('hpwallart', '$headers from multi_add_to_cart ids = '.$project_ids , $all_headers, WATCHDOG_NOTICE);

  global $base_url;
  drupal_goto($base_url . '/cart');
  exit;
}