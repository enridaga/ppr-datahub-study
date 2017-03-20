<?php
require_once 'default.inc';
class TaskController extends DefaultController {
	private $_task = NULL;
	private $_errors = array();
	protected function _init() {
		// Validate Task and User
		$taskId = $this->get ( 'id' );
		if($taskId){
			if(DataAccess::instance()->userHasTask($this->_userid(), $taskId)){
				$this->_task = $taskId;
			}else{
				$this->_forbidden("Not a user's task.");
			}
		}else{
			$this->_forbidden("No task specified.");
		}
	}

	public function indexAction() {
		$this->taskAction();
	}
	
	public function submitchoicesAction(){
		if (! $this->_task){
			$this->_malformed ( "Missing task" );
		}
		
		$posted = $this->get('choice', array());
		$choices = DataAccess::instance()->policies( $this->_task );
		foreach($choices as $output => $choice):
			foreach($choice as $deontic => $actions):
				foreach($actions as $action => $datasets):
					if(!isset($posted[$output][$deontic][$action])){
						$this->_errors['choice'][$output][$deontic][$action] = 'Missing';
					} 
				endforeach;
			endforeach;
		endforeach;
		
		if(empty($this->_errors)){
			// Write data
			DataAccess::instance()->choices($this->_userid(), $this->_task, $posted );
			// Move to phase 5
			$this->moveonAction();
		}else{
			// Render
			$m = $this->phaseModel(4);
			// Edit with posted data
			$m['data']['choice'] = $posted;
			$v = new TaskViewer( $m );
			$v->render ( 'layout' );
		}
	}

	public function taskAction() {
		if (! $this->_task){
			$this->_malformed ( "Missing task" );
		}
		// Get Phase
		$phase = DataAccess::instance()->phase($this->_userid(), $this->_task);
		
		// Depending on phase do phase ...
		if($phase == NULL){
			$phase = 1;
			DataAccess::instance()->phase($this->_userid(), $this->_task, $phase);
		}
		
		// Render
		$m = $this->phaseModel($phase);
		$v = new TaskViewer( $m );
		$v->render ( 'layout' );
	}
	
	public function phaseModel($phase){
		
		$descriptions = array(
			1 => 'Understand the process',
			2 => 'Understand the input datasets',
			3 => 'Understand how process/datasets interplay',
			4 => 'Indicate what policies propagate to the output',
			5 => 'Compare with the automatic reasoner',
			6 => 'Task completed'
		);
		
		$m = $this->model();
		$m ['data'] ['id'] = $this->_task;
		$m ['data'] ['phasedescription'] = $descriptions [$phase];
		$m ['data'] ['phasevisited'] = DataAccess::instance()->phaseVisited($this->_userid(), $this->_task);
		$m ['data'] ['descriptions'] = $descriptions;
		if(!empty($this->_errors)){
			if(isset($this->_errors['choice'])){
				$_SESSION['messages'] = array('warning' => array('Please fill up all the rows'));
			}	
		}
		$m ['data'] ['task'] = DataAccess::instance ()->task ( $this->_task );
		$m ['title'] = $m ['data'] ['task']->label;
		if($phase > 1){
			$m ['data'] ['datasets'] = DataAccess::instance ()->datasets ( $this->_task );
		}
		if($phase >= 3){
			$m['data'] ['mappings'] = DataAccess::instance()->inputs( $this->_task );
			foreach ($m['data']['mappings'] as $map){
				if(isset($m['data']['datasets'][$map[0]])){
					$m['data']['datasets'][$map[0]]['mapping'] = $map[2];
				}
			}
			$m['data'] ['outputs'] = DataAccess::instance()->outputs( $this->_task );
		}
		if($phase >= 4){
			$m['data'] ['policies'] = DataAccess::instance()->policies( $this->_task );
			$m['data'] ['choice'] = DataAccess::instance()->choices( $this->_userid(), $this->_task );
		}
		if($phase == 5){
			// Test with automatic output
			$m['data'] ['propagated'] = DataAccess::instance()->propagated( $this->_task );
			$m['data'] ['conflicts'] = DataAccess::instance()->conflicts( $this->_userid(), $this->_task );
			$resolved = TRUE;
			foreach($m['data'] ['conflicts'] as $props){
				foreach($props as $prop){
					foreach($prop as $conf){
						if(!$conf['status']){
							$resolved = FALSE;
							break(2);
						}
					}
				}
			};
			$m['data'] ['resolved'] = $resolved; 
		}
		$m ['data'] ['phase'] = $phase;
		$m ['page'] = 'task';
		return $m;
	}
	
	public function completeAction(){
		DataAccess::instance()->phase($this->_userid(), $this->_task, DataAccess::PHASE_COMPLETE);
		$this->_redirect('tasks', 'index');
	}
	
	public function downloadAction() {
		header("Content-type: application/xml");
		header("Content-Disposition: attachment; filename=" . $this->_task.".rmp");
		// Download the rapid miner workflow of the given action
		$fslocation = DataAccess::instance()->getDownloadFile($this->_task);
		$this->streamFile($fslocation);
	}
	
	public function stepbackAction() {
		// Step to the previous phase ...?
		if($this->_isPOST()){
			$phase = DataAccess::instance()->phase($this->_userid(), $this->get('id'));
			DataAccess::instance()->phase($this->_userid(), $this->get('id'), $phase - 1);
			$this->_redirect('task','task', array('id'=>$this->get('id')));
		}else{
			$this->_badmethod();
		}
	}
	private function _jumpTo($phase){
		// Step to the previous phase ...?
		if($this->_isPOST()){
			DataAccess::instance()->phase($this->_userid(), $this->get('id'), $phase);
			$this->_redirect('task','task', array('id'=>$this->get('id')));
		}else{
			$this->_badmethod();
		}
	}
	public function jumpto1Action() { 
			$this->_jumpTo(1);
	}
	
	public function jumpto2Action() {$this->_jumpTo(2); }
	public function jumpto3Action() { $this->_jumpTo(3);}
	public function jumpto4Action() { $this->_jumpTo(4);}
	public function jumpto5Action() { $this->_jumpTo(5);}
	public function jumpto6Action() { $this->_jumpTo(6);}
	public function moveonAction() {
		// Step to the previous phase ...?
		if($this->_isPOST()){
			$phase = DataAccess::instance()->phase($this->_userid(), $this->get('id'));
			DataAccess::instance()->phase($this->_userid(), $this->get('id'), $phase + 1);
			$this->_redirect('task','task', array('id'=>$this->get('id')));
		}else{
			$this->_badmethod();
		}
	}
	
	
}

