<?php

require_once 'default.inc';
class AdminController extends DefaultController {
	protected function _init() {
		// Validate Task and User
		if($this->_userid() !== 1000){
			$this->_forbidden("No task specified.");
		}
	}
	
	public function indexAction(){
		$this->tasksAction();	
	}
	
	public function tasksAction(){
		
// 		$p = DataAccess::instance()->rules()->propagates('http://purl.org/datanode/ns/hasCopy','permission http://purl.org/NET/ldr/ns#extraction');
		
// 		var_dump($p);die;
		
		$m = $this->model ();
		$m ['data'] ['tasks'] = array ();
		$m ['data'] ['tasks'] = DataAccess::instance ()->tasks ();
		$m ['data'] ['status'] = DataAccess::instance()->tasksUsersStatus();
		$m ['data'] ['number-of-policies'] = DataAccess::instance()->numberOfPolicies();
		$m ['data'] ['propagated'] = DataAccess::instance()->propagatedPolicies();
		$m ['page'] = 'tasks';
		$m ['title'] = "Tasks overview";
		$m ['data'] ['admin'] = TRUE;
		$v = new TaskViewer ( $m );
		$v->render ( 'layout' );
	}
	
	public function datanodeAction(){

		$m = $this->model ();
		$m['data']['relations'] = DataAccess::instance ()->relations ();
		$rules = DataAccess::instance()->rules();
		$relations = array_keys($m['data']['relations']);
		$m['data']['num-pprs'] = $rules->numberOfRules($relations);
		$dataflows = DataAccess::instance()->dataflows();
		$numInFlows = array();
		foreach($relations as $rel){
			$numInFlows[$rel] = $dataflows->triplesWith(NULL,$rel);
		}
		
		$m ['data']['num-in-flows'] = $numInFlows;
		$m ['data']['subs'] = DataAccess::instance()->subproperties($relations);
		$m ['data']['iof'] = DataAccess::instance()->inverses($relations);
		$m ['title'] = 'Datanode Stats';
		$m['page'] = 'datanode';
		$v = new TaskViewer ( $m );
		$v->render ( 'layout' );
	}
	
	public function policiesAction(){
		$m = $this->model ();
		$m ['data'] ['admin'] = TRUE;
		$m ['page'] = 'adminpolicies';
		$m ['title'] = "Policies overview";
		$m ['data'] ['policies'] = DataAccess::instance()->listPolicies();
		$v = new TaskViewer ( $m );
		$v->render ( 'layout' );
	}
	

	public function dataflowAction(){
		$m = $this->model();
		$taskId = $this->get('id');
		$task = DataAccess::instance()->task($taskId);
		$m ['data'] ['id'] = $taskId;
		$m['data']['task'] = $task;
		$m['data']['width'] = 1000;
		$m['data']['height'] = 1000;
		$m['title'] = $task->label;
		$m['data']['trace'] = DataAccess::instance()->sankey($taskId)->process();
		$m['page'] = 'process';
		$v = new TaskViewer( $m );
		$v->render ( 'layout' );
	}
}