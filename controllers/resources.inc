<?php
class ResourcesController extends MVC_Controller {
	public function stylesAction() {
		header ( "Content-type: text/css; charset=utf8" );
		print file_get_contents ( dirname ( __FILE__ ) . '/../styles/' . $this->get ( 'id' ) . '.css' );
	}
	public function d3Action() {
		header ( "Content-type: application/javascript; charset=utf8" );
		print file_get_contents ( dirname ( __FILE__ ) . '/../d3/d3.min.js' );
	}
	public function jsAction() {
		if (! $this->has ( 'name' ))
			error_malformed ( 'Missing parameter: name ');
		$name = $this->get ( 'name' );
		header ( "Content-type: application/javascript; charset=utf8" );
		print file_get_contents ( dirname ( __FILE__ ) . '/../js/' . $name );
	}
}