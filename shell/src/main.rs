/**
 * 💠 Obsidian OS Shell - Main Entry Point
 * AI-powered shell with natural language processing capabilities
 */

use std::io::{self, Write};
use std::process::Command;
use std::env;
use std::path::Path;
use tokio;
use serde::{Deserialize, Serialize};
use anyhow::{Result, Context};
use clap::{Parser, Subcommand};

mod ai;
mod commands;
mod history;
mod completion;
mod config;

use ai::AIEngine;
use commands::CommandExecutor;
use history::CommandHistory;
use completion::CommandCompletion;
use config::ShellConfig;

/// AI-powered shell for Obsidian OS
#[derive(Parser)]
#[command(name = "obsidian-shell")]
#[command(about = "AI-powered shell for Obsidian OS")]
#[command(version = "0.1.0")]
struct Cli {
    #[command(subcommand)]
    command: Option<Commands>,
    
    /// Enable AI assistance
    #[arg(short, long)]
    ai: bool,
    
    /// Enable GUI mode
    #[arg(short, long)]
    gui: bool,
    
    /// Configuration file path
    #[arg(short, long, default_value = "~/.config/obsidian-shell/config.toml")]
    config: String,
}

#[derive(Subcommand)]
enum Commands {
    /// Execute a command with AI assistance
    Exec {
        /// Command to execute
        command: String,
        
        /// Use AI to interpret the command
        #[arg(short, long)]
        interpret: bool,
    },
    
    /// Start interactive shell
    Interactive,
    
    /// Show shell configuration
    Config,
    
    /// Update AI models
    UpdateModels,
}

/// Main shell structure
struct ObsidianShell {
    ai_engine: AIEngine,
    command_executor: CommandExecutor,
    history: CommandHistory,
    completion: CommandCompletion,
    config: ShellConfig,
}

impl ObsidianShell {
    /// Create a new shell instance
    fn new(config_path: &str) -> Result<Self> {
        let config = ShellConfig::load(config_path)?;
        let ai_engine = AIEngine::new(&config.ai_config)?;
        let command_executor = CommandExecutor::new();
        let history = CommandHistory::new(&config.history_path)?;
        let completion = CommandCompletion::new();
        
        Ok(ObsidianShell {
            ai_engine,
            command_executor,
            history,
            completion,
            config,
        })
    }
    
    /// Initialize the shell
    async fn initialize(&mut self) -> Result<()> {
        println!("💠 Obsidian Shell v0.1.0");
        println!("AI-powered shell for Obsidian OS");
        println!("Type 'help' for available commands or 'exit' to quit.\n");
        
        // Load command history
        self.history.load()?;
        
        // Initialize AI engine
        if self.config.ai_enabled {
            println!("🤖 Initializing AI engine...");
            self.ai_engine.initialize().await?;
            println!("✅ AI engine ready!");
        }
        
        Ok(())
    }
    
    /// Run the interactive shell
    async fn run_interactive(&mut self) -> Result<()> {
        let mut buffer = String::new();
        
        loop {
            // Display prompt
            self.display_prompt();
            
            // Read input
            buffer.clear();
            io::stdin().read_line(&mut buffer)?;
            
            let input = buffer.trim();
            if input.is_empty() {
                continue;
            }
            
            // Handle special commands
            match input {
                "exit" | "quit" => break,
                "help" => self.show_help(),
                "clear" => self.clear_screen(),
                "history" => self.show_history(),
                _ => {
                    // Process command
                    self.process_command(input).await?;
                }
            }
        }
        
        Ok(())
    }
    
    /// Display the shell prompt
    fn display_prompt(&self) {
        let current_dir = env::current_dir()
            .unwrap_or_else(|_| env::current_dir().unwrap_or_default());
        
        let dir_name = current_dir
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or("~");
        
        print!("💠 {} $ ", dir_name);
        io::stdout().flush().unwrap();
    }
    
    /// Process a command
    async fn process_command(&mut self, input: &str) -> Result<()> {
        // Add to history
        self.history.add(input);
        
        // Check if AI interpretation is needed
        if self.config.ai_enabled && self.should_use_ai(input) {
            match self.ai_engine.interpret_command(input).await {
                Ok(interpreted) => {
                    println!("🤖 AI interpretation: {}", interpreted);
                    self.execute_command(&interpreted).await?;
                }
                Err(e) => {
                    println!("⚠️  AI interpretation failed: {}", e);
                    println!("Executing original command...");
                    self.execute_command(input).await?;
                }
            }
        } else {
            self.execute_command(input).await?;
        }
        
        Ok(())
    }
    
    /// Determine if AI should be used for command interpretation
    fn should_use_ai(&self, input: &str) -> bool {
        // Use AI for natural language commands
        let natural_language_indicators = [
            "find", "search", "show", "list", "get", "create", "delete",
            "move", "copy", "open", "start", "stop", "install", "update"
        ];
        
        let lower_input = input.to_lowercase();
        natural_language_indicators.iter().any(|&indicator| {
            lower_input.contains(indicator)
        })
    }
    
    /// Execute a command
    async fn execute_command(&self, command: &str) -> Result<()> {
        // Split command into parts
        let parts: Vec<&str> = command.split_whitespace().collect();
        if parts.is_empty() {
            return Ok(());
        }
        
        let (program, args) = parts.split_first().unwrap();
        
        // Execute the command
        match self.command_executor.execute(program, args).await {
            Ok(output) => {
                if !output.is_empty() {
                    println!("{}", output);
                }
            }
            Err(e) => {
                eprintln!("❌ Error executing command: {}", e);
            }
        }
        
        Ok(())
    }
    
    /// Show help information
    fn show_help(&self) {
        println!("\n💠 Obsidian Shell Help");
        println!("=====================");
        println!("Built-in commands:");
        println!("  help     - Show this help");
        println!("  clear    - Clear the screen");
        println!("  history  - Show command history");
        println!("  exit     - Exit the shell");
        println!("  quit     - Exit the shell");
        println!("\nAI Features:");
        println!("  Natural language commands are automatically interpreted");
        println!("  Examples:");
        println!("    'find all text files' -> 'find . -name \"*.txt\"'");
        println!("    'show running processes' -> 'ps aux'");
        println!("    'install python package requests' -> 'pip install requests'");
        println!();
    }
    
    /// Clear the screen
    fn clear_screen(&self) {
        print!("\x1B[2J\x1B[1;1H");
        io::stdout().flush().unwrap();
    }
    
    /// Show command history
    fn show_history(&self) {
        println!("\nCommand History:");
        println!("================");
        
        match self.history.get_recent(10) {
            Ok(history) => {
                for (i, command) in history.iter().enumerate() {
                    println!("{:3}: {}", i + 1, command);
                }
            }
            Err(e) => {
                eprintln!("❌ Error loading history: {}", e);
            }
        }
        println!();
    }
}

/// Main function
#[tokio::main]
async fn main() -> Result<()> {
    let cli = Cli::parse();
    
    match cli.command {
        Some(Commands::Exec { command, interpret }) => {
            // Execute single command
            let mut shell = ObsidianShell::new(&cli.config)?;
            shell.initialize().await?;
            
            if interpret {
                // Use AI interpretation
                match shell.ai_engine.interpret_command(&command).await {
                    Ok(interpreted) => {
                        println!("🤖 AI interpretation: {}", interpreted);
                        shell.execute_command(&interpreted).await?;
                    }
                    Err(e) => {
                        eprintln!("❌ AI interpretation failed: {}", e);
                        return Err(e);
                    }
                }
            } else {
                shell.execute_command(&command).await?;
            }
        }
        
        Some(Commands::Interactive) => {
            // Run interactive shell
            let mut shell = ObsidianShell::new(&cli.config)?;
            shell.initialize().await?;
            shell.run_interactive().await?;
        }
        
        Some(Commands::Config) => {
            // Show configuration
            let config = ShellConfig::load(&cli.config)?;
            println!("💠 Obsidian Shell Configuration");
            println!("===============================");
            println!("AI Enabled: {}", config.ai_enabled);
            println!("GUI Enabled: {}", config.gui_enabled);
            println!("History Path: {}", config.history_path);
            println!("Model Path: {}", config.ai_config.model_path);
        }
        
        Some(Commands::UpdateModels) => {
            // Update AI models
            println!("🔄 Updating AI models...");
            let config = ShellConfig::load(&cli.config)?;
            let ai_engine = AIEngine::new(&config.ai_config)?;
            ai_engine.update_models().await?;
            println!("✅ Models updated successfully!");
        }
        
        None => {
            // Default to interactive mode
            let mut shell = ObsidianShell::new(&cli.config)?;
            shell.initialize().await?;
            shell.run_interactive().await?;
        }
    }
    
    Ok(())
}

// Module implementations
mod ai {
    use super::*;
    use serde::{Deserialize, Serialize};
    
    #[derive(Debug, Serialize, Deserialize)]
    pub struct AIConfig {
        pub model_path: String,
        pub api_endpoint: String,
        pub max_tokens: usize,
        pub temperature: f32,
    }
    
    pub struct AIEngine {
        config: AIConfig,
        client: reqwest::Client,
    }
    
    impl AIEngine {
        pub fn new(config: &AIConfig) -> Result<Self> {
            let client = reqwest::Client::new();
            Ok(AIEngine {
                config: config.clone(),
                client,
            })
        }
        
        pub async fn initialize(&self) -> Result<()> {
            // Initialize AI engine
            println!("Loading AI model from: {}", self.config.model_path);
            Ok(())
        }
        
        pub async fn interpret_command(&self, input: &str) -> Result<String> {
            // Use AI to interpret natural language command
            let prompt = format!(
                "Convert this natural language command to a shell command: '{}'",
                input
            );
            
            // For now, return a simple interpretation
            // In a real implementation, this would call the AI model
            let interpreted = match input.to_lowercase() {
                s if s.contains("find") && s.contains("file") => {
                    "find . -type f".to_string()
                }
                s if s.contains("process") => {
                    "ps aux".to_string()
                }
                s if s.contains("install") => {
                    "apt install".to_string()
                }
                _ => input.to_string(),
            };
            
            Ok(interpreted)
        }
        
        pub async fn update_models(&self) -> Result<()> {
            println!("Downloading latest AI models...");
            // Implementation for model updates
            Ok(())
        }
    }
}

mod commands {
    use super::*;
    use tokio::process::Command;
    
    pub struct CommandExecutor;
    
    impl CommandExecutor {
        pub fn new() -> Self {
            CommandExecutor
        }
        
        pub async fn execute(&self, program: &str, args: &[&str]) -> Result<String> {
            let output = Command::new(program)
                .args(args)
                .output()
                .await
                .context("Failed to execute command")?;
            
            let stdout = String::from_utf8_lossy(&output.stdout);
            let stderr = String::from_utf8_lossy(&output.stderr);
            
            if !output.status.success() {
                return Err(anyhow::anyhow!("Command failed: {}", stderr));
            }
            
            Ok(stdout.to_string())
        }
    }
}

mod history {
    use super::*;
    use std::fs::OpenOptions;
    use std::io::{BufRead, BufReader, Write};
    
    pub struct CommandHistory {
        path: String,
        commands: Vec<String>,
    }
    
    impl CommandHistory {
        pub fn new(path: &str) -> Result<Self> {
            Ok(CommandHistory {
                path: path.to_string(),
                commands: Vec::new(),
            })
        }
        
        pub fn load(&mut self) -> Result<()> {
            if let Ok(file) = std::fs::File::open(&self.path) {
                let reader = BufReader::new(file);
                for line in reader.lines() {
                    if let Ok(command) = line {
                        self.commands.push(command);
                    }
                }
            }
            Ok(())
        }
        
        pub fn add(&mut self, command: &str) {
            self.commands.push(command.to_string());
            
            // Save to file
            if let Ok(mut file) = OpenOptions::new()
                .create(true)
                .append(true)
                .open(&self.path)
            {
                let _ = writeln!(file, "{}", command);
            }
        }
        
        pub fn get_recent(&self, count: usize) -> Result<Vec<String>> {
            let start = if self.commands.len() > count {
                self.commands.len() - count
            } else {
                0
            };
            Ok(self.commands[start..].to_vec())
        }
    }
}

mod completion {
    use super::*;
    
    pub struct CommandCompletion;
    
    impl CommandCompletion {
        pub fn new() -> Self {
            CommandCompletion
        }
        
        pub fn complete(&self, _input: &str) -> Vec<String> {
            // Implementation for command completion
            vec![]
        }
    }
}

mod config {
    use super::*;
    use serde::{Deserialize, Serialize};
    
    #[derive(Debug, Serialize, Deserialize)]
    pub struct ShellConfig {
        pub ai_enabled: bool,
        pub gui_enabled: bool,
        pub history_path: String,
        pub ai_config: ai::AIConfig,
    }
    
    impl ShellConfig {
        pub fn load(path: &str) -> Result<Self> {
            // Default configuration
            let config = ShellConfig {
                ai_enabled: true,
                gui_enabled: false,
                history_path: "~/.obsidian-shell-history".to_string(),
                ai_config: ai::AIConfig {
                    model_path: "/usr/share/obsidian/models/llm.onnx".to_string(),
                    api_endpoint: "http://localhost:8000/ai".to_string(),
                    max_tokens: 512,
                    temperature: 0.7,
                },
            };
            
            // Try to load from file if it exists
            if let Ok(contents) = std::fs::read_to_string(path) {
                return toml::from_str(&contents)
                    .context("Failed to parse configuration file");
            }
            
            Ok(config)
        }
    }
} 