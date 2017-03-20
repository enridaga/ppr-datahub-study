<?php
/**
 * MVC TOOLS
 * 
 * This is a light MVC framework.
 * It is unlicensed and free.
 * 
 * @author enridaga
 *
 */

class MVC_Container {
	protected $_parameters = array();
	public function setParam($key, $value){
		$this->_parameters[$key] = $value;
	}

	public function getParam($key, $default = FALSE){
		return $this->get($key, $default);
	}

	public function has($key){
		return isset($this->_parameters[$key]) ? TRUE : FALSE;
	}

	public function full($key){
		return ($this->has($key) && ( $this->get($key) != '' ) );
	}

	public function get($key, $default = FALSE){
		if( $this->has($key) ) {
			return $this->_parameters[$key];
		}else{
			return $default;
		}
	}

	public function getAll(){
		return $this->_parameters;
	}

	/**
	 *
	 * @param unknown $param
	 * @param unknown $itemKey
	 * @param string $default
	 */
	public function itemOf($key,$itemKey, $default = NULL){
		if($this->has($key) && is_array($this->get($key))){
			$a = $this->get($key);
			if(array_key_exists($itemKey, $a)){
				return $a[$itemKey];
			}
		}
		return $default;
	}
}
abstract class MVC_Controller extends MVC_Container{

	protected $_action_parameter = 'action';
	// 	protected $_parameters = array();
	protected $_validators = array();
	protected $_sanitizers = array();
	public function __construct($args = array()){
		if(isset($args['action_parameter'])){
			$this->_action_parameter = $args['action_parameter'];
		}
		if(isset($args['parameters'])){
			$this->_parameters = $args['parameters'];
		}
		// Register shared validators
		$this->setValidator('NotNull', new MVC_NotNullVal());
		$this->setValidator('NotEmpty', new MVC_NotEmptyVal());
		$this->setValidator('Int', new MVC_IntVal());
		
	}

	public function indexAction(){
		// Nothing to do
	}



	public function run(){
		if(isset($this->_parameters[$this->_action_parameter])){
			$this->perform($this->_parameters[$this->_action_parameter]);
		}else{
			$this->perform('index');
		}
	}

	public function perform($action){
		$method = $action . 'Action';
		if(method_exists($this, $method)){
			$this->$method();
		}else{
			$this->indexAction();
		}
	}

	public function setValidator($name, MVC_Validator $validator){
		$this->_validators[$name] = $validator;
	}

	public function setSanitizer($name, MVC_Sanitizer $sanitizer){
		$this->_sanitizers[$name] = $sanitizer;
	}

	public function getValidator($name){
		return $this->_validators[$name];
	}

	public function getSanitizer(){
		return $this->_sanitizers[$name];
	}

	public function validate($validators){
		$errors = array();
		foreach($this->_parameters as $param => $value){
			if(isset($validators[$param])){
				foreach($validators[$param] as $validator){
					if($validator instanceof MVC_Validator){
						$tester = $validator;
					}else{
						$tester = $this->getValidator($validator);
					}
					$result = $tester->test($value);
					if($result !== TRUE){
						if(!isset($error[$param])){
							$errors[$param] = array();
						}
						array_push($errors[$param], $result);
					}
				}
			}
		}
		return $errors;
	}

	private function applySan($result, $san){
		if($san instanceof MVC_Sanitizer){
			$result = $san->filter($result);
		}else if(function_exists($san)){
			$result = @$san($result);
		}else{
			$sanitizer = $this->getSanitizer($san);
			$result = $sanitizer->filter($result);
		}
		return $result;
	}

	public function sanitize($sanitizers){
		$errors = array();
		foreach($this->_parameters as $param => $value){
			if(isset($sanitizers[$param])){
				$result = $value;
				if(is_array($sanitizers[$param])){
					foreach($sanitizers[$param] as $san){
						$result = $this->applySan($result, $san);
					}
				}else{
					$result = $this->applySan($result, $sanitizers[$param]);
				}
				$this->_parameters[$param] = $result;
			}
		}
	}
}


interface MVC_Sanitizer{
	public function filter($val);
}

interface MVC_Validator{
	public function test($val);
}

class MVC_OneOfVal implements MVC_Validator{
	private $_mode = NULL;
	private $_values = NULL;
	public function __construct($values, $mode = TRUE){
		$this->_values = $values;
		$this->_mode = $mode;
	}
	public function test($val){
		if (in_array($val, $this->_values) === $this->_mode)
			return TRUE;
		return 'Value ' . ( $this->_mode ? 'not in' : ' in ' ) . ' "' . join(', ', $this->_values) . '"';
	}
	public function allowed(){
		$this->_mode = TRUE;
	}
	public function forbidden(){
		$this->_mode = FALSE;
	}
}

class MVC_CallbackVal implements MVC_Validator{
	private $_function = NULL;
	public function __construct($func){
		if(function_exists($func)){
			$this->_function = $func;
		} else throw new Exception('Function ' . $func . ' does not exist!');
	}

	public function test($val){
		return $this->_function($val);
	}
}
class MVC_NotNullVal implements MVC_Validator{
	public function test($value){
		if($value === NULL){
			return "Cannot be null";
		}
		return TRUE;
	}
}
class MVC_NotEmptyVal implements MVC_Validator{
	public function test($value){
		if(empty($value) || $value == ''){
			return "Cannot be empty";
		}
		return TRUE;
	}
}
class MVC_IntVal implements MVC_Validator{
	public function test($value){
		if(strval(intval($value)) !== $value){
			return "Must be int, was: " . $value;
		}
		return TRUE;
	}
}

class MVC_Viewer extends MVC_Container{
	private $_templateDirectory;

	public function __construct ( $data = array() ){
		$this->_templateDirectory = dirname(__FILE__) . '/html';
		$this->_parameters = $data;
	}
	public function render($template){
		include $this->_templateDirectory . '/' . $template . '.phtml';
	}

	public function partial($template, $data = array()){
		$class = get_class($this);
		$viewer = new $class($data);
		$viewer->render($template);
	}

	/**
	 * Prints the value of $key
	 * @param string $key
	 */
	public function spit($key){
		print $this->get($key);
	}

	/**
	 * Prints the value of $key, encode special chars as HTML entities
	 * @param string $key
	 */
	public function smushed($key){
		print $this->smush($this->get($key));
	}

	/**
	 * Returns the passed value, encode special chars as HTML entities
	 * @param string $value
	 */
	public static function smush($value){
		return htmlspecialchars($value);
	}
	
	public function spitMany(array $keys = array(), $sep = ' '){
		$first = TRUE;
		foreach($kets as $key){
			if($first) $first = false;
			else print $sep;
			$this->spit($key);
		}
	}
	
}

class MVC_Html extends MVC_Viewer{
	public function spitAttr($key, $attr = ''){
		if($this->has($key)){
			if($attr == ''){
				$attr = $key;
			}
			print ' ' . $attr .'="' ;
			$this->spit($key) ;
			print '" ';
		}
	}

	public function spitAttrs(array $attrs = array('id', 'name', 'placeholder', 'class', 'style', 'type'), $prefix = ''){
		foreach($attrs as $attr){
			$_attr = $prefix . $attr;
			$this->spitAttr($_attr, $attr);
		}
	}
}