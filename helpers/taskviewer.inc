<?php 

class TaskViewer extends MVC_Html{
	public function localName($uri){
		if(($pos = strpos($uri, '#')) !== FALSE){
			return substr($uri, $pos+1);
		}else{
			return substr($uri, strrpos($uri, '/')+1);
		}
	}
	public function cropped($s,$n){
		if(strlen($s) <= $n){
			return $s;
		}
		$s=substr($s, 0,$n-3).' ...';
		return $s;
	}
}