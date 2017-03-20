<?php
require_once 'default.inc';
class HelpController extends DefaultController {
	public function dnAction(){
		print json_encode(DataAccess::instance()->dninfo($this->get('relation')), true);
	}
	
}