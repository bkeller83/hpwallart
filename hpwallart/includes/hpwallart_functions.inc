<?php

/**
 * @file
 * Functions for the hpwallart module
 */

// Define Constants
$hpwallart_base_url = variable_get('hpwallart_base_url', 'SET-ME');
define('HPWALLART_BASE_URL', $hpwallart_base_url);

/**
 * Create a 16 digit auth token
 * Ensure that it's unique by checking table users
 * Then save it to the users database
 */
function hpwallart_create_token($uid) {
  do {
      $token = "";
      $codeAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      $codeAlphabet.= "abcdefghijklmnopqrstuvwxyz";
      $codeAlphabet.= "0123456789";
      $max = strlen($codeAlphabet) - 1;
      for ($i = 0; $i < 16; $i++) {
         $token .= $codeAlphabet[mt_rand(0, $max)];
      }
      $query = db_query('SELECT uid from {users} WHERE hp_auth_token = :token', array('token' => $token));
      $result = $query->fetchAll();
  } while (!empty($result));
  // We have a unique token, save it to this user's uid
  db_update('users')->fields(array('hp_auth_token' => $token))->condition('uid', $uid, '=')->execute();
  return $token;
}

/**
 * Get a user id from an auth token
 * @param string $token An HP WallArt Suite User's token
 * @return string User ID
 */
function hpwallart_get_user_id_by_token($token) {
  $query = db_query('SELECT uid FROM {users} WHERE hp_auth_token = :token', array(':token' => $token));
  $result = $query->fetchField();

  return $result;
}

function getallheaders_manual() {
  $result = array();
  foreach ($_SERVER as $key => $value) {
    if (substr($key, 0, 5) == "HTTP_") {
      $key = str_replace(" ", "-", ucwords(strtolower(str_replace("_", " ", substr($key, 5)))));
      $result[$key] = $value;
    } else {
      $result[$key] = $value;
    }
  }
  return $result;
}


/**
 * Return an array of projects for a given user id
 * @param string $uid user id (drupal)
 * @return array of project arrays, or empty array
 */
function hpwallart_get_user_projects($uid) {
  $query = db_query('SELECT * FROM {hpwallart_projects} WHERE user_id = :uid', array(':uid' => $uid));
  $result = $query->fetchAll();
  // $result is now an array of objects (or empty)
  // This system needs an array of arrays
  if (!empty($result)) {
    $projects = array();
    foreach ($result as $project) {
      $p = (array) $project;
      $projects[] = $p;
    }
    return $projects;
  } else {
    return $result;
  }
}

/**
 * Pulls a project from our DB by project id
 * @param type $id
 * @return project object $project
 */
function hpwallart_get_project_by_id($id) {
  $query = db_query('SELECT * FROM {hpwallart_projects} WHERE id = :id', array(':id' => $id));
  $result = $query->fetchObject();
  return new HPWallArtProject($result);
}

/**
 * Create a table listing projects given a user id
 * @param type $uid Drupal user id
 * @todo context token in edit link
 */
function hpwallart_projects_table($uid) {
  global $user;
  if ($user->uid !== $uid) {
    return FALSE;
  }
  $projects = hpwallart_get_user_projects($user->uid);
  // Create a table for our projects display
  $table['header'] = array(
    array('data' => 'Preview', 'data-sort' => 'float', 'class' => 'sortable'),
    array('data' => 'HP Order ID', 'data-sort' => 'float', 'class' => 'sortable'),
    array('data' =>'Project Name', 'data-sort' => 'string', 'class' => 'sortable'),
    array('data' =>'Type', 'data-sort' => 'string', 'class' => 'sortable'),
    array('data' =>'State', 'data-sort' => 'string', 'class' => 'sortable'),
    array('data' =>'Price', 'data-sort' => 'float', 'class' => 'sortable'),
    array('data' =>'Actions',),
    );
  $table['rows'] = array();
  $table['attributes'] = array('id' => 'hpwallart-projects-table');
  $table['caption'] = 'Your HP WallArt Projects';
  $table['colgroups'] = array();
  $table['sticky'] = TRUE;
  $table['empty'] = 'No Projects Were Found!';
  $sku_array = array('WA_WP' => 'Poster', 'WA_WC' => 'Wall', 'WA_CA' => 'Canvas', 'WA_WD' => 'Decal',);

  foreach ($projects as $project) {
    $img_small = 'https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/' . $project['id'] . '/preview_small.png'; // 180px
    $p = file_get_contents('https://store.hpwallart.com/' . HPWALLART_BASE_URL . '/projects/'.$project['id']);
    $proj = json_decode($p);
    $price = hpwallart_get_project_obj_price($proj->price);
    $table['rows'][] = array(
      '<img src="' . $img_small . '" />',
      $proj->order_id,
      $project['name'],
      $sku_array[$proj->product_descriptor->product_sku],
      $project['state'],
      '$' . $price,
      '<a href="https://designer.hpwallart.com/'.HPWALLART_BASE_URL.'?web_link=true&project_id='.$project['id'].'">Edit</a> | ' .
      '<a href="https://designer.hpwallart.com/'.HPWALLART_BASE_URL.'?web_link=true&project_id='.$project['id'].'&preview=true&preview_size=large" target="_blank">Preview</a>',
    );
  }
  return $table;
}

/**
 * HP stores the price in an object (when pulled in with jsone_decode) of objects
 * So we'll take the base object price from a project object, and filter through it
 * adding up all the prices as we go...
 * @param object $price
 */
function hpwallart_get_project_obj_price($price) {
  $total = 0;
  if (is_array($price)) {
    $price = (object)$price;
  }
  if (is_object($price)) {
    foreach ($price as $key => $ptype) {
      if (property_exists($ptype, 'price')) {
        $total += (float)$ptype->price;
      }
    }
    return $total;
  } else {
    return FALSE;
  }
}

/**
 * Admin Settings Form Builder
 */
function hpwallart_admin_settings($form, &$form_state) {
  $form['base_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Base URL'),
    '#description' => t('This is your "url_identifier" as configured in your HP WallArt Suite settings.'),
    '#default_value' => HPWALLART_BASE_URL,
    '#name' => 'base_url',
   );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
    '#submit' => array('hpwallart_settings_page'),
    '#executes_submit_callback' => FALSE,
  );
  return $form;
}


class HPWallArtProject {

  public $id;//                      int(11)       No		Auto numeric. Primary key
  public $name;//                    varchar(255)	No    (documentation had project_name but that does not seem to work)
  public $width;//                   varchar(255)	No
  public $height;//                  varchar(255)	No
  public $user_id;//                 int(11)       No		Foreign key, related to the user table
  public $scene;// (1...n)           varchar(255)	No		Each of the background images (escenas)
  public $element;// (1...n)         varchar(255)	No		Each of the vector objects to place on top of the background (elementos)
  public $price;//                   float(0.00) 	No		Base price for square meter
  public $content_context_token;//   varchar(255)	No		Unique identifier to pass to designer by GET
  public $path;//                    varchar(255)	No		Project folder (PATH????)
  public $format;//                  varchar(255)	No		Format of the project SKU (WP | WA | CV)
  public $state ;//                  varchar(255)	No		State of the project in designer (EDITING | IN_CART | PURCHASED | DELETED)
  public $has_pixelation_warning;//  tinyint

  /* Setters and Getters */
  public function getId(){
		return $this->id;
	}
	public function setId($id){
		$this->id = $id;
	}
	public function getName(){
		return $this->name;
	}
	public function setName($name){
		$this->name = $name;
	}
	public function getWidth(){
		return $this->width;
	}
	public function setWidth($width){
		$this->width = $width;
	}
	public function getHeight(){
		return $this->height;
	}
	public function setHeight($height){
		$this->height = $height;
	}
	public function getUser_id(){
		return $this->user_id;
	}
	public function setUser_id($user_id){
		$this->user_id = $user_id;
	}
	public function getScene(){
		return $this->scene;
	}
	public function setScene($scene){
		$this->scene = $scene;
	}
	public function getElement(){
		return $this->element;
	}
	public function setElement($element){
		$this->element = $element;
	}
	public function getPrice(){
		return $this->price;
	}
	public function setPrice($price){
		$this->price = $price;
	}
	public function getContent_context_token(){
		return $this->content_context_token;
	}
	public function setContent_context_token($content_context_token){
		$this->content_context_token = $content_context_token;
	}
	public function getPath(){
		return $this->path;
	}
	public function setPath($path){
		$this->path = $path;
	}
	public function getFormat(){
		return $this->format;
	}
	public function setFormat($format){
		$this->format = $format;
	}
	public function getState(){
		return $this->state;
	}
	public function setState($state){
		$this->state = $state;
  }
  public function getPixelWarning() {
    return $this->has_pixelation_warning;
  }
  public function setPixelWarning($warning) {
    $this->has_pixelation_warning = $warning;
  }
  /**
   * Construct takes an OBJECT containing id, name, path, and content_context_token
   * Note that for a new project, the id will be nil
   * @param object $project
   * Somehow we need to compare the $project object to the current values stored in the db so we dont set current stuff as null...
   */
  public function __construct($project) {
    if (property_exists($project, 'id')) {
      // Get the current project stored in the DB
      $query = db_query('SELECT * FROM hpwallart_projects WHERE id = :id', array(':id' => $project->id));
      $db_project = $query->fetchObject();
      // If the project object we are creating has a null value, load the value in from the database project object
      // this way, it will either be set to the current value in the DB, or remain null
      foreach ($db_project as $key => $value) {
        if ( (property_exists($project, $key)  && $project->$key === NULL) || !property_exists($project, $key)) {
          $project->$key = $value;
        }
      }
    } else {
      $project->id = NULL;
    }
    // Ensure properties exist
    $project->name = property_exists($project, 'name') ? $project->name : NULL;
    $project->width = property_exists($project, 'width') ? $project->width : NULL;
    $project->height = property_exists($project, 'height') ? $project->height : NULL;
    $project->user_id = property_exists($project, 'user_id') ? $project->user_id : NULL;
    $project->scene = property_exists($project, 'scene') ? $project->scene : NULL;
    $project->element = property_exists($project, 'element') ? $project->element : NULL;
    $project->price = property_exists($project, 'price') ? $project->price : NULL;
    $project->content_context_token = property_exists($project, 'content_context_token') ? $project->content_context_token : NULL;
    $project->path = property_exists($project, 'path') ? $project->path : NULL;
    $project->format = property_exists($project, 'format') ? $project->format : NULL;
    $project->state = property_exists($project, 'state') ? $project->state : 'EDITING';
    $project->has_pixelation_warning = property_exists($project, 'has_pixelation_warning') ? $project->has_pixelation_warning : NULL;

    $this->setId($project->id);
    $this->setName($project->name);
    $this->setWidth($project->width);
    $this->setHeight($project->height);
    $this->setUser_id($project->user_id);
    $this->setScene($project->scene);
    $this->setElement($project->element);
    $this->setPrice($project->price);
    $this->setContent_context_token($project->content_context_token);
    $this->setPath($project->path);
    $this->setFormat($project->format);
    $this->setState($project->state);
    $this->setPixelWarning($project->has_pixelation_warning);
  }

 /**
  * Saves a new project to the DB and returns the new project id
  * @param string $user_id Drupal user's id
  * @param array $project The project array we need to save
  * @return string $project_id the project id from the
 */
  public function save_new_project($user_id) {
    $project_id = db_insert('hpwallart_projects')
        ->fields(array(
          'path' => $this->getPath(),
          'name' => $this->getName(),
          'content_context_token' => $this->getContent_context_token(),
          'user_id' => $user_id,
        ))
        ->execute();
    return $project_id;
  }

  public function save_project() {
    // Make sure the price is not an object
    if (is_object($this->getPrice())) {
      $price = hpwallart_get_project_obj_price($this->getPrice());
    } else {
      $price = $this->getPrice();
    }
    $this->getPixelWarning() ? $pw = 1 : $pw = 0;
    $result = db_update('hpwallart_projects')
        ->fields(array(
            'name' => $this->getName(),
            'width' => $this->getWidth(),
            'height' => $this->getHeight(),
            'scene' => $this->getScene(),
            'element' => $this->getElement(),
            'price' => $price,
            'content_context_token' => $this->getContent_context_token(),
            'path' => $this->getPath(),
            'format' => $this->getFormat(),
            'state' => $this->getState(),
            'has_pixelation_warning' => $pw,
        ))
        ->condition('id', $this->getId(), '=')
        ->execute();

  }

  public function json_echo() {
    $array = (array)$this;
    $echo_array = array();
    foreach ($array as $key => $value) {
      if (!is_null($value)) {
        $echo_array[$key] = $value;
      }
    }
    echo json_encode($echo_array);
  }

  /** End of Class **/
}

