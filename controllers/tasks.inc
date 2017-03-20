<?php
require_once 'default.inc';
class TasksController extends DefaultController {

	protected function _init() {
		
	}
	public function indexAction() {
		$m = $this->model ();
		$m ['data'] ['tasks'] = array ();
		$m ['data'] ['tasks'] = DataAccess::instance ()->tasks ( $this->_userid () );
		foreach($m['data']['tasks'] as &$task){
			$task->visited = DataAccess::instance()->phaseVisited($this->_userid(), $task->id);
		}
		$m ['page'] = 'tasks';
		$v = new MVC_Viewer ( $m );
		$v->render ( 'layout' );
	}
}