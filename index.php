<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);
ini_set ( 'max_execution_time', 1200);
ini_set ( 'memory_limit', "512M");
require_once ('mvctools.inc');

function controller($controller){
	require_once ('controllers/' . $controller . '.inc');
	return ucfirst($controller . 'Controller');
}

function route($controller = 'default', $action = 'index', $params = array()){
	$parameters = '';
	foreach($params as $k => $v){
		$parameters .= '&' . $k .'=' . $v ;
	}
	return '?controller=' . $controller . '&action=' . $action . $parameters;
}

function router(){
	$params = array_merge($_GET, $_POST );
	$controller = 'default';
	if(isset($params['controller'])){
		$controller = $params['controller'];
	}else{
		$params['controller'] = $controller;
	}
	$action = 'index';
	if(isset($params['action'])){
		$action = $params['action'];
	}else{
		$params['action'] = $action;
	}
	return array(
		'controller' => $controller ,
		'action' => $action,
		'parameters' => $params );
}

function main(){
	require_once 'http.inc';
	try{
		$routing = router();
		$action = $routing['action'];
		$controllerClass = controller($routing['controller']);
		$controller = new $controllerClass (array('parameters' => $routing['parameters']));
		$controller->run();
	}catch(Exception $e){
		error_internal_server_error($e->getMessage(), $e);
	}
}
main();