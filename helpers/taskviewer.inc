<?php 

class TaskViewer extends MVC_Html{
	public function localName($uri){
		if(($pos = strpos($uri, '#')) !== FALSE){
			return substr($uri, $pos+1);
		}else{
			return substr($uri, strrpos($uri, '/')+1);
		}
	}
}