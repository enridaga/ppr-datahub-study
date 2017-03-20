<?php
function error_not_found($msg) {
	header("HTTP/1.0 404 Not Found", true, 404);
	header("Content-type: text/plain; charset=utf-8");
	echo 'Not Found ' . "\n\n";
	echo print_error($msg);
	exit();
}
function error_malformed($msg){
	header("HTTP/1.0 400 Bad request", true, 400);
	header("Content-type: text/plain; charset=utf-8");
	echo 'Bad request ' . "\n\n";
	echo print_error($msg);
	exit();
}
function error_method_not_allowed($method, $msg = NULL){
	header("HTTP/1.0 405 Method Not Allowed", true, 405);
	header("Content-type: text/plain; charset=utf-8");
	echo 'Method not allowed: ' . $method . "\n\n";
	echo print_error($msg == NULL ? $echo : $msg);
	exit();
}
function error_internal_server_error($msg, Exception $e = NULL){
	header("HTTP/1.0 500 Internal Server Error", true, 500);
	header("Content-type: text/plain; charset=utf-8");
	echo 'Server error' . "\n";
	echo print_error($msg, $e);
	exit();
}
function error_forbidden($message){
	header("HTTP/1.0 403 Forbidden", true, 403);
	header("Content-type: text/plain; charset=utf-8");
	echo 'Forbidden: ' . $message;
	exit();
}
function print_error($msg, Exception $e = NULL){
	$echo = '';
	if(is_array($msg)){
		foreach($msg as $m){
			$echo .= "\n". print_r($m, TRUE);
		}
	}else{
		$echo .=  "\n" . $msg;
	}
	if($e !== NULL){
		$echo .= "\n\n".  $e->getTraceAsString();
	}
	error_log($echo);
	return $echo;

}
function redirect($location, $code = '303'){
	header("Location: $location", 303);
	exit;
}