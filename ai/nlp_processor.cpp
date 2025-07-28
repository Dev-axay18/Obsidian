#include <iostream>
#include <string>

extern "C" {
    int nlp_processor_init() {
        std::cout << "NLP Processor initialized" << std::endl;
        return 0;
    }
    
    int nlp_processor_process_text(const char* text) {
        // Placeholder for text processing
        return 0;
    }
    
    const char* nlp_processor_get_intent() {
        return "NLP intent placeholder";
    }
    
    void nlp_processor_cleanup() {
        std::cout << "NLP Processor cleaned up" << std::endl;
    }
} 